#include "qe_bridge.hpp"

#include "setup_input.hpp"

#include <algorithm>
#include <cstddef>

namespace perfect6502_debug
{
namespace
{
constexpr std::size_t MemorySize = 65536u;

std::uint8_t clean_p(std::uint8_t p)
{
    return static_cast<std::uint8_t>((p & 0xcfu) | 0x20u);
}

} // namespace

QeMachine::QeMachine()
{
    cpu_.model = qe6502_model_nmos;
    fill_memory(0xeau);
}

bool QeMachine::apply_setup(const SetupInput& setup, std::string& error)
{
    fill_memory(setup.memory_fill_value);

    for (const MemoryWrite& write : setup.memory_writes)
    {
        if (write.bytes.empty())
        {
            error = "cannot apply an empty memory write";
            return false;
        }

        const std::uint32_t address = write.address;
        const std::uint32_t bytes_available = static_cast<std::uint32_t>(MemorySize) - address;
        if (write.bytes.size() > bytes_available)
        {
            error = "memory write crosses the end of the 64K address space";
            return false;
        }

        for (std::size_t i = 0u; i < write.bytes.size(); ++i)
        {
            write_memory(static_cast<std::uint16_t>(address + i), write.bytes[i]);
        }
    }

    cpu_ = qe6502_t{};
    cpu_.model = qe6502_model_nmos;
    cpu_.A = setup.registers.a;
    cpu_.X = setup.registers.x;
    cpu_.Y = setup.registers.y;
    cpu_.S = setup.registers.s;
    cpu_.P = clean_p(setup.registers.p);
    visible_registers_.pc = setup.registers.pc;
    visible_registers_.a = setup.registers.a;
    visible_registers_.x = setup.registers.x;
    visible_registers_.y = setup.registers.y;
    visible_registers_.s = setup.registers.s;
    visible_registers_.p = cpu_.P;
    visible_registers_.ir = setup.registers.ir;
    side_effects_pending_ = false;

    start_at_pc(setup.registers.pc);
    initialized_ = true;
    return true;
}

void QeMachine::start_at_pc(std::uint16_t pc)
{
    /* qe6502_goto returns the pending opcode read request for the requested PC. */
    tick_ = qe6502_goto(&cpu_, pc);
    data_bus_ = 0u;
    ir_ = 0u;
    half_cycle_ = 0u;
    before_memory_half_ = true;
}

QeSnapshot QeMachine::create_snapshot(std::string& error) const
{
    if (!initialized_)
    {
        error = "cannot snapshot an uninitialized qe6502 machine";
        return QeSnapshot();
    }

    QeSnapshot snapshot;
    snapshot.cpu = cpu_;
    snapshot.tick = tick_;
    snapshot.memory = memory_;
    snapshot.data_bus = data_bus_;
    snapshot.ir = ir_;
    snapshot.visible_registers = visible_registers_;
    snapshot.side_effects_pending = side_effects_pending_;
    snapshot.half_cycle = half_cycle_;
    snapshot.before_memory_half = before_memory_half_;
    snapshot.valid = true;
    return snapshot;
}

bool QeMachine::restore_snapshot(const QeSnapshot& snapshot, std::string& error)
{
    if (!snapshot.valid)
    {
        error = "cannot restore from an empty qe6502 snapshot";
        return false;
    }

    cpu_ = snapshot.cpu;
    tick_ = snapshot.tick;
    memory_ = snapshot.memory;
    data_bus_ = snapshot.data_bus;
    ir_ = snapshot.ir;
    visible_registers_ = snapshot.visible_registers;
    side_effects_pending_ = snapshot.side_effects_pending;
    half_cycle_ = snapshot.half_cycle;
    before_memory_half_ = snapshot.before_memory_half;
    initialized_ = true;
    return true;
}

void QeMachine::step_half_cycle()
{
    if (before_memory_half_)
    {
        if ((tick_.status & qe6502_status_writing) != 0u)
        {
            memory_[tick_.address] = tick_.bus;
            data_bus_ = tick_.bus;
        }
        else
        {
            data_bus_ = memory_[tick_.address];
        }
    }
    else
    {
        const bool input_is_opcode_fetch = (tick_.status & qe6502_status_opcode_fetch) != 0u;
        const bool side_effects_were_pending = side_effects_pending_;
        const std::uint16_t input_address = tick_.address;
        const std::uint16_t visible_pc_before_tick = visible_registers_.pc;

        if (input_is_opcode_fetch)
        {
            ir_ = data_bus_;
            visible_registers_.pc = static_cast<std::uint16_t>(input_address + 1u);
            visible_registers_.ir = data_bus_;
        }

        tick_ = qe6502_tick(&cpu_, data_bus_);

        if (!input_is_opcode_fetch)
        {
            visible_registers_.pc = cpu_.PC;
        }

        if ((cpu_.A != visible_registers_.a) ||
            (cpu_.X != visible_registers_.x) ||
            (cpu_.Y != visible_registers_.y) ||
            (cpu_.S != visible_registers_.s) ||
            (cpu_.P != visible_registers_.p))
        {
            side_effects_pending_ = true;
        }

        const bool output_is_new_opcode_fetch =
            ((tick_.status & qe6502_status_opcode_fetch) != 0u) &&
            (tick_.address != input_address);
        const bool input_consumes_early_opcode_fetch =
            input_is_opcode_fetch &&
            side_effects_were_pending &&
            (input_address == visible_pc_before_tick);

        if (output_is_new_opcode_fetch || input_consumes_early_opcode_fetch)
        {
            publish_instruction_side_effects();
        }
    }

    before_memory_half_ = !before_memory_half_;
    half_cycle_++;
}

void QeMachine::step_full_cycle()
{
    step_half_cycle();
    step_half_cycle();
}

void QeMachine::publish_instruction_side_effects()
{
    visible_registers_.pc = cpu_.PC;
    visible_registers_.a = cpu_.A;
    visible_registers_.x = cpu_.X;
    visible_registers_.y = cpu_.Y;
    visible_registers_.s = cpu_.S;
    visible_registers_.p = cpu_.P;
    side_effects_pending_ = false;
}

CpuRegisters QeMachine::read_registers() const
{
    return visible_registers_;
}

std::uint16_t QeMachine::read_address_bus() const
{
    return tick_.address;
}

std::uint8_t QeMachine::read_data_bus() const
{
    if ((tick_.status & qe6502_status_writing) != 0u)
    {
        return tick_.bus;
    }

    return data_bus_;
}

bool QeMachine::is_read() const
{
    return (tick_.status & qe6502_status_writing) == 0u;
}

std::uint64_t QeMachine::half_cycle() const
{
    return half_cycle_;
}

InterruptInputs QeMachine::read_interrupt_inputs() const
{
    InterruptInputs inputs;
    inputs.irq_asserted = qe6502_get_irq(&cpu_) == 0u;
    inputs.nmi_asserted = qe6502_get_nmi(&cpu_) == 0u;
    return inputs;
}

void QeMachine::set_irq_asserted(bool asserted)
{
    qe6502_set_irq(&cpu_, asserted ? 0u : 1u);
}

void QeMachine::set_nmi_asserted(bool asserted)
{
    qe6502_set_nmi(&cpu_, asserted ? 0u : 1u);
}

std::uint8_t QeMachine::read_memory(std::uint16_t address) const
{
    return memory_[address];
}

void QeMachine::write_memory(std::uint16_t address, std::uint8_t value)
{
    memory_[address] = value;
}

void QeMachine::fill_memory(std::uint8_t value)
{
    std::fill(memory_.begin(), memory_.end(), value);
}

} // namespace perfect6502_debug
