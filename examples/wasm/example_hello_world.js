import { loadQE6502, QE6502_MODEL_MOS } from "./qe6502.js";

export async function run({ output }) {
  function print(message = "") {
    output.textContent += message + "\n";
  }

  function hex8(value) {
    return "0x" + (value & 0xff).toString(16).padStart(2, "0");
  }

  function hex16(value) {
    return "0x" + (value & 0xffff).toString(16).padStart(4, "0");
  }

  function assertEqual(name, actual, expected) {
    if (actual !== expected) {
      throw new Error(
        `${name}: expected ${hex16(expected)}, got ${hex16(actual)}`,
      );
    }

    print(`OK: ${name} = ${hex16(actual)}`);
  }

  try {
    output.textContent = "";

    const qe = await loadQE6502(`./qe6502.wasm?v=${Date.now()}`, {
      debugLog: (topic, message) => {
        console.debug(`[QE6502:${topic}] ${message}`);
      },
    });

    const cpu = qe.createCPU();
    const ram = new Uint8Array(0x10000);

    const PROGRAM_START = 0x8000;
    const PROGRAM_END = 0x800e;

    /*
      Program at $8000:

        LDA #$10       ; A = $10
        TAX            ; X = A
        INX            ; X = $11
        STX $0200      ; RAM[$0200] = $11
        INX            ; X = $12
        LDA #$77       ; A = $77
        STA $0201      ; RAM[$0201] = $77
        NOP            ; final instruction

      Expected after execution:

        A = $77
        X = $12
        RAM[$0200] = $11
        RAM[$0201] = $77
        PC = $800D
    */

    ram[0x8000] = 0xa9; // LDA #$10
    ram[0x8001] = 0x10;

    ram[0x8002] = 0xaa; // TAX

    ram[0x8003] = 0xe8; // INX

    ram[0x8004] = 0x8e; // STX $0200
    ram[0x8005] = 0x00;
    ram[0x8006] = 0x02;

    ram[0x8007] = 0xe8; // INX

    ram[0x8008] = 0xa9; // LDA #$77
    ram[0x8009] = 0x77;

    ram[0x800a] = 0x8d; // STA $0201
    ram[0x800b] = 0x01;
    ram[0x800c] = 0x02;

    ram[0x800d] = 0xea; // NOP

    // Reset vector -> $8000
    ram[0xfffc] = PROGRAM_START & 0xff;
    ram[0xfffd] = PROGRAM_START >> 8;

    cpu.powerOn(QE6502_MODEL_MOS);

    function serviceBusBeforeTick() {
      if (cpu.needsData()) {
        const addr = cpu.address();
        const value = ram[addr];

        cpu.feedData(value);
      }

      if (cpu.hasData()) {
        const addr = cpu.address();
        const value = cpu.readData();

        ram[addr] = value;
      }
    }

    function tick() {
      serviceBusBeforeTick();
      cpu.tick();
    }

    function runUntilProgramEnd(maxTicks = 1000) {
      for (let i = 0; i < maxTicks; i++) {
        tick();

        const regs = cpu.dump();

        if (cpu.isInstrDone() && regs.pc === PROGRAM_END) {
          return {
            ticks: i + 1,
            regs,
          };
        }
      }

      throw new Error("Program did not finish");
    }

    print("Starting QE6502 program...");
    print(`Program start: ${hex16(PROGRAM_START)}`);
    print(`Program end:   ${hex16(PROGRAM_END)}`);
    print("");

    const result = runUntilProgramEnd();

    print(`Program finished after ${result.ticks} ticks`);
    print("");

    const regs = cpu.dump();

    print("Final registers:");
    print(`PC = ${hex16(regs.pc)}`);
    print(`A  = ${hex8(regs.a)}`);
    print(`X  = ${hex8(regs.x)}`);
    print(`Y  = ${hex8(regs.y)}`);
    print(`S  = ${hex8(regs.s)}`);
    print(`P  = ${hex8(regs.p)}`);
    print("");

    print("Checking results:");

    assertEqual("PC", regs.pc, 0x800e);
    assertEqual("A", regs.a, 0x77);
    assertEqual("X", regs.x, 0x12);
    assertEqual("RAM[$0200]", ram[0x0200], 0x11);
    assertEqual("RAM[$0201]", ram[0x0201], 0x77);

    print("");
    print("SUCCESS: 6502 program executed correctly.");

    cpu.dispose();
  } catch (error) {
    output.textContent += "\nERROR:\n";
    output.textContent += error instanceof Error ? error.stack : String(error);
  }
}
