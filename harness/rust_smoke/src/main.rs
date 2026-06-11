use qe6502::{Cpu, Model, Tick, TICK_OPCODE_FETCH, TICK_WRITE};

const PROGRAM_ADDRESS: u16 = 0x8000;
const STORE_ADDRESS: u16 = 0x0200;

fn main() {
    if let Err(err) = run_smoke() {
        eprintln!("qe6502 Rust smoke: FAIL");
        eprintln!("{err}");
        std::process::exit(1);
    }

    println!("qe6502 Rust smoke: PASS");
}

fn run_smoke() -> Result<(), Box<dyn std::error::Error>> {
    println!("qe6502 Rust smoke: starting");

    let mut cpu = Cpu::new(Model::Nmos)?;
    let mut memory = [0u8; 65_536];

    memory[0x8000] = 0xA9; // LDA #$42
    memory[0x8001] = 0x42;
    memory[0x8002] = 0x8D; // STA $0200
    memory[0x8003] = 0x00;
    memory[0x8004] = 0x02;
    memory[0x8005] = 0x4C; // JMP $8000
    memory[0x8006] = 0x00;
    memory[0x8007] = 0x80;

    cpu.jump_to(PROGRAM_ADDRESS);
    let tick = cpu.last_tick();
    println!(
        "Initial tick: address=0x{:04X} data=0x{:02X} write={}",
        tick.address,
        tick.data,
        is_write(tick)
    );

    require(
        tick.address == PROGRAM_ADDRESS,
        "JumpTo did not produce the expected fetch address.",
    )?;
    require(!is_write(tick), "Initial JumpTo tick must be a read cycle.")?;
    require(is_opcode_fetch(tick), "Initial JumpTo tick must be an opcode fetch.")?;

    run_bus_cycles(&mut cpu, &mut memory, 32);

    println!(
        "After 32 cycles: mem[0x0200]=0x{:02X} A=0x{:02X} PC=0x{:04X}",
        memory[STORE_ADDRESS as usize],
        cpu.a(),
        cpu.pc()
    );

    require(
        memory[STORE_ADDRESS as usize] == 0x42,
        "Program did not store 0x42 at 0x0200.",
    )?;
    require(cpu.a() == 0x42, "Accumulator did not keep the loaded value.")?;

    check_register_accessors(&mut cpu)?;
    check_status_flag_accessors(&mut cpu)?;
    check_interrupt_accessors(&mut cpu)?;
    check_save_load(&mut cpu)?;

    Ok(())
}

fn run_bus_cycles(cpu: &mut Cpu, memory: &mut [u8; 65_536], cycle_count: usize) {
    let mut tick = cpu.last_tick();
    for _ in 0..cycle_count {
        let address = tick.address as usize;
        let data = if is_write(tick) {
            memory[address] = tick.data;
            0
        } else {
            memory[address]
        };
        tick = cpu.tick(data);
    }
}

fn check_register_accessors(cpu: &mut Cpu) -> Result<(), String> {
    cpu.set_pc(0x3456);
    cpu.set_s(0xFD);
    cpu.set_a(0x12);
    cpu.set_x(0x34);
    cpu.set_y(0x56);
    cpu.set_p(0xA5);

    require(cpu.pc() == 0x3456, "PC setter/getter failed.")?;
    require(cpu.s() == 0xFD, "S setter/getter failed.")?;
    require(cpu.a() == 0x12, "A setter/getter failed.")?;
    require(cpu.x() == 0x34, "X setter/getter failed.")?;
    require(cpu.y() == 0x56, "Y setter/getter failed.")?;
    require(cpu.p() == 0xA5, "P setter/getter failed.")?;

    Ok(())
}

fn check_status_flag_accessors(cpu: &mut Cpu) -> Result<(), String> {
    cpu.set_p(0);

    cpu.set_carry_flag(true);
    cpu.set_zero_flag(true);
    cpu.set_interrupt_disable_flag(true);
    cpu.set_decimal_flag(true);
    cpu.set_break_flag(true);
    cpu.set_unused_flag(true);
    cpu.set_overflow_flag(true);
    cpu.set_negative_flag(true);

    require(cpu.carry_flag(), "Carry flag setter/getter failed.")?;
    require(cpu.zero_flag(), "Zero flag setter/getter failed.")?;
    require(
        cpu.interrupt_disable_flag(),
        "Interrupt-disable flag setter/getter failed.",
    )?;
    require(cpu.decimal_flag(), "Decimal flag setter/getter failed.")?;
    require(cpu.break_flag(), "Break flag setter/getter failed.")?;
    require(cpu.unused_flag(), "Unused flag setter/getter failed.")?;
    require(cpu.overflow_flag(), "Overflow flag setter/getter failed.")?;
    require(cpu.negative_flag(), "Negative flag setter/getter failed.")?;

    cpu.set_carry_flag(false);
    cpu.set_zero_flag(false);
    cpu.set_interrupt_disable_flag(false);
    cpu.set_decimal_flag(false);
    cpu.set_break_flag(false);
    cpu.set_unused_flag(false);
    cpu.set_overflow_flag(false);
    cpu.set_negative_flag(false);

    require(!cpu.carry_flag(), "Carry flag clear failed.")?;
    require(!cpu.zero_flag(), "Zero flag clear failed.")?;
    require(!cpu.interrupt_disable_flag(), "Interrupt-disable flag clear failed.")?;
    require(!cpu.decimal_flag(), "Decimal flag clear failed.")?;
    require(!cpu.break_flag(), "Break flag clear failed.")?;
    require(!cpu.unused_flag(), "Unused flag clear failed.")?;
    require(!cpu.overflow_flag(), "Overflow flag clear failed.")?;
    require(!cpu.negative_flag(), "Negative flag clear failed.")?;

    Ok(())
}

fn check_interrupt_accessors(cpu: &mut Cpu) -> Result<(), String> {
    cpu.set_nmi_asserted(true);
    cpu.set_irq_asserted(true);
    require(cpu.nmi_asserted(), "NMI asserted setter/getter failed.")?;
    require(cpu.irq_asserted(), "IRQ asserted setter/getter failed.")?;

    cpu.set_nmi_asserted(false);
    cpu.set_irq_asserted(false);
    require(!cpu.nmi_asserted(), "NMI deassert failed.")?;
    require(!cpu.irq_asserted(), "IRQ deassert failed.")?;

    Ok(())
}

fn check_save_load(cpu: &mut Cpu) -> Result<(), String> {
    cpu.set_a(0x42);
    cpu.set_pc(0x8003);
    let snapshot = cpu.save();
    let saved_a = cpu.a();
    let saved_pc = cpu.pc();
    let saved_tick = cpu.last_tick();

    cpu.set_a(0x00);
    cpu.set_pc(0x1234);
    let loaded_tick = cpu.load(&snapshot);

    require(cpu.a() == saved_a, "Save/Load did not restore A.")?;
    require(cpu.pc() == saved_pc, "Save/Load did not restore PC.")?;
    require(loaded_tick == saved_tick, "Load did not return the saved tick.")?;
    require(cpu.last_tick() == saved_tick, "Save/Load did not restore the last tick.")?;

    println!(
        "Snapshot restore: A=0x{:02X} PC=0x{:04X} tick={{address=0x{:04X}, data=0x{:02X}, flags=0x{:02X}}}",
        cpu.a(),
        cpu.pc(),
        loaded_tick.address,
        loaded_tick.data,
        loaded_tick.flags
    );

    Ok(())
}

#[inline(always)]
fn is_write(tick: Tick) -> bool {
    (tick.flags & TICK_WRITE) != 0
}

#[inline(always)]
fn is_opcode_fetch(tick: Tick) -> bool {
    (tick.flags & TICK_OPCODE_FETCH) != 0
}

fn require(condition: bool, message: &str) -> Result<(), String> {
    if condition {
        Ok(())
    } else {
        Err(message.to_owned())
    }
}
