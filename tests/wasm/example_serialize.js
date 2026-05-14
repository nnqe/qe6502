import {
  loadQE6502,
  QE6502_MODEL_MOS,
} from "./qe6502.js";

export async function run({ output }) {
  function print(message = "") {
    output.textContent += message + "\n";
  }

  function hex8(value) {
    return "0x" + (value & 0xff).toString(16).padStart(2, "0").toUpperCase();
  }

  function hex16(value) {
    return "0x" + (value & 0xffff).toString(16).padStart(4, "0").toUpperCase();
  }

  const qe = await loadQE6502(`./qe6502.wasm?v=${Date.now()}`);

  const ram = new Uint8Array(0x10000);
  const PROGRAM_START = 0x8000;

  /*
    Program:

      $8000: LDX #$10
      $8002: INX
      $8003: STX $0200

    Test flow:

      1. Run first complete instruction: LDX #$10
      2. Serialize CPU registers at instruction boundary
      3. Destroy CPU
      4. Create new CPU
      5. Restore registers
      6. Run INX
      7. Verify X == $11
      8. Run STX $0200
      9. Verify RAM[$0200] == $11

    Important:
      CPU serialization is supported only when the current instruction
      has fully completed. Serializing in the middle of an instruction
      is not supported by this example.
  */

  ram[0x8000] = 0xa2; // LDX #$10
  ram[0x8001] = 0x10;

  ram[0x8002] = 0xe8; // INX

  ram[0x8003] = 0x8e; // STX $0200
  ram[0x8004] = 0x00;
  ram[0x8005] = 0x02;

  // Reset vector -> $8000
  ram[0xfffc] = PROGRAM_START & 0xff;
  ram[0xfffd] = PROGRAM_START >>> 8;

  function serviceBusBeforeTick(cpu) {
    if (cpu.needsData()) {
      const addr = cpu.address();
      cpu.feedData(ram[addr]);
    }

    if (cpu.hasData()) {
      const addr = cpu.address();
      ram[addr] = cpu.readData();
    }
  }

  function tick(cpu) {
    serviceBusBeforeTick(cpu);
    cpu.tick();
  }

  function runUntilPc(cpu, expectedPc, maxTicks = 1000) {
    for (let i = 0; i < maxTicks; i++) {
      tick(cpu);

      const regs = cpu.readRegs();

      if (cpu.isInstrDone() && regs.pc === expectedPc) {
        return regs;
      }
    }

    throw new Error(`Did not reach PC ${hex16(expectedPc)}`);
  }

  print("QE6502 serialization example");
  print("=".repeat(72));
  print("");
  print("Serialization rule:");
  print("Only serialize when cpu.isInstrDone() is true.");
  print("Mid-instruction serialization is not supported.");
  print("");

  print("Creating first CPU...");
  let cpu = qe.createCPU();
  cpu.powerOn(QE6502_MODEL_MOS);

  print("Running first instruction: LDX #$10");
  let regs = runUntilPc(cpu, 0x8002);

  print(`After first instruction: PC=${hex16(regs.pc)} X=${hex8(regs.x)}`);

  if (!cpu.isInstrDone()) {
    throw new Error("CPU is not at instruction boundary");
  }

  if (regs.x !== 0x10) {
    throw new Error(`Expected X = 0x10 after LDX, got ${hex8(regs.x)}`);
  }

  print("");
  print("Serializing CPU registers...");
  const serializedRegs = cpu.readRegs();

  print(`Serialized PC=${hex16(serializedRegs.pc)}`);
  print(`Serialized X =${hex8(serializedRegs.x)}`);

  print("Destroying first CPU...");
  cpu.dispose();

  print("");
  print("Creating second CPU...");
  cpu = qe.createCPU();

  /*
    Power on first so the internal CPU object is initialized.
    Then overwrite the public register state from the serialized snapshot.
  */
  cpu.powerOn(QE6502_MODEL_MOS);

  print("Restoring CPU registers...");
  cpu.overwriteRegs(serializedRegs);

  regs = cpu.readRegs();

  print(`Restored PC=${hex16(regs.pc)}`);
  print(`Restored X =${hex8(regs.x)}`);
  print("");

  if (regs.pc !== 0x8002) {
    throw new Error(`Expected restored PC = 0x8002, got ${hex16(regs.pc)}`);
  }

  if (regs.x !== 0x10) {
    throw new Error(`Expected restored X = 0x10, got ${hex8(regs.x)}`);
  }

  print("Running second instruction: INX");
  regs = runUntilPc(cpu, 0x8003);

  print(`After INX: PC=${hex16(regs.pc)} X=${hex8(regs.x)}`);

  if (regs.x !== 0x11) {
    throw new Error(`Expected X = 0x11 after INX, got ${hex8(regs.x)}`);
  }

  print("");
  print("Running third instruction: STX $0200");
  regs = runUntilPc(cpu, 0x8006);

  print(`After STX: PC=${hex16(regs.pc)} X=${hex8(regs.x)}`);
  print(`RAM[$0200] = ${hex8(ram[0x0200])}`);

  if (ram[0x0200] !== 0x11) {
    throw new Error(`Expected RAM[$0200] = 0x11, got ${hex8(ram[0x0200])}`);
  }

  if (!cpu.ok()) {
    throw new Error(`CPU error code: ${cpu.errorCode()}`);
  }

  cpu.dispose();

  print("");
  print("SUCCESS: CPU was serialized, destroyed, restored, and continued correctly.");
}