import {
  loadQE6502,
  QE6502_MODEL_MOS,
  QE6502_MODEL_WDC,
  QE6502_MODEL_RW,
  QE6502_MODEL_ST,
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

  function formatNumber(value) {
    return value.toLocaleString("en-US");
  }

  async function loadHexRom(url) {
    const response = await fetch(url);

    if (!response.ok) {
      throw new Error(
        `Failed to fetch ${url}: ${response.status} ${response.statusText}`,
      );
    }

    const text = await response.text();

    const matches = text.match(/0x[0-9a-fA-F]{1,2}|\b\d+\b/g);

    if (!matches) {
      throw new Error(`No byte values found in ${url}`);
    }

    const bytes = matches.map((token) => {
      const value =
        token.startsWith("0x") || token.startsWith("0X")
          ? Number.parseInt(token, 16)
          : Number.parseInt(token, 10);

      if (!Number.isInteger(value) || value < 0 || value > 0xff) {
        throw new Error(`Invalid byte value in ${url}: ${token}`);
      }

      return value;
    });

    if (bytes.length !== 0x10000) {
      throw new Error(
        `${url}: expected 65,536 bytes, got ${formatNumber(bytes.length)}`,
      );
    }

    return new Uint8Array(bytes);
  }

  function serviceBusBeforeTick(cpu, memory) {
    const address = cpu.address();

    if (cpu.needsData()) {
      cpu.feedData(memory[address]);
    } else if (cpu.hasData()) {
      memory[address] = cpu.readData();
    }
  }

  function tick(cpu, memory) {
    serviceBusBeforeTick(cpu, memory);
    cpu.tick();
  }

  function printRegs(regs) {
    print(`PC     = ${hex16(regs.pc)}`);
    print(`A      = ${hex8(regs.a)}`);
    print(`X      = ${hex8(regs.x)}`);
    print(`Y      = ${hex8(regs.y)}`);
    print(`S      = ${hex8(regs.s)}`);
    print(`P      = ${hex8(regs.p)}`);
  }

  async function runKlausTest(qe, options) {
    const { name, model, romUrl, successAddress, expectedInstructions } =
      options;

    print("=".repeat(72));
    print(name);
    print("=".repeat(72));

    print(`ROM:                   ${romUrl}`);
    print(`Success address:       ${hex16(successAddress)}`);
    print(`Expected instructions: ${formatNumber(expectedInstructions)}`);
    print("");

    const memory = await loadHexRom(romUrl);

    // The original C harness forces reset vector to $0400.
    memory[0xfffc] = 0x00;
    memory[0xfffd] = 0x04;

    const cpu = qe.createCPU();

    let instructions = 0;
    let ticks = 0;

    try {
      cpu.powerOn(model);

      // Match the C harness: first tick until the CPU reports it has started.
      while (!cpu.isStarted()) {
        tick(cpu, memory);
        ticks++;
      }

      const startedAt = performance.now();

      while (cpu.ok()) {
        const address = cpu.address();

        if (address === successAddress) {
          const elapsedMs = performance.now() - startedAt;
          const elapsedSeconds = elapsedMs / 1000;
          const ticksPerSecond = ticks / elapsedSeconds;
          const mhz = ticksPerSecond / 1_000_000;

          if (instructions !== expectedInstructions) {
            throw new Error(
              `Reached success address ${hex16(successAddress)}, ` +
                `but expected ${formatNumber(expectedInstructions)} instructions, ` +
                `got ${formatNumber(instructions)}`,
            );
          }

          print("PASS");
          print("");
          print(`Instructions:          ${formatNumber(instructions)}`);
          print(`Ticks:                 ${formatNumber(ticks)}`);
          print(`Elapsed:               ${elapsedMs.toFixed(2)} ms`);
          print(`Average tick speed:    ${mhz.toFixed(2)} MHz`);
          print("");

          print("Final registers:");
          printRegs(cpu.dump());
          print("");

          return {
            passed: true,
            instructions,
            ticks,
            elapsedMs,
            regs: cpu.dump(),
          };
        }

        tick(cpu, memory);
        ticks++;

        if (cpu.isInstrDone()) {
          instructions++;

          if (instructions > expectedInstructions * 2) {
            throw new Error(
              `Instruction limit exceeded: ${formatNumber(instructions)}`,
            );
          }
        }
      }

      throw new Error(`CPU failed with error code: ${cpu.errorCode()}`);
    } finally {
      cpu.dispose();
    }
  }

  async function main() {
    output.textContent = "";

    print("Loading QE6502 WASM...");

    const qe = await loadQE6502(`./qe6502_js.wasm?v=${Date.now()}`, {
      debugLog: (topic, message) => {
        console.debug(`[QE6502:${topic}] ${message}`);
      },
    });

    print("Loaded.");
    print("");

    const tests = [
      {
        name: "RW 65C02 functional test",
        model: QE6502_MODEL_RW,
        romUrl: "./6502_functional_test.hex",
        successAddress: 0x3469,
        expectedInstructions: 30_646_176,
      },
      {
        name: "RW 65C02 extended opcodes test",
        model: QE6502_MODEL_RW,
        romUrl: "./65C02_extended_opcodes_test.hex",
        successAddress: 0x24f1,
        expectedInstructions: 21_986_985,
      },
      {
        name: "WDC 65C02 extended opcodes test",
        model: QE6502_MODEL_WDC,
        romUrl: "./65C02_extended_opcodes_test.hex",
        successAddress: 0x24f1,
        expectedInstructions: 21_986_985,
      },
      {
        name: "ST 65C02 functional test",
        model: QE6502_MODEL_ST,
        romUrl: "./6502_functional_test.hex",
        successAddress: 0x3469,
        expectedInstructions: 30_646_176,
      },
      {
        name: "WDC 65C02 functional test",
        model: QE6502_MODEL_WDC,
        romUrl: "./6502_functional_test.hex",
        successAddress: 0x3469,
        expectedInstructions: 30_646_176,
      },
      {
        name: "MOS 6502 functional test",
        model: QE6502_MODEL_MOS,
        romUrl: "./6502_functional_test.hex",
        successAddress: 0x3469,
        expectedInstructions: 30_646_176,
      },
    ];

    const allStartedAt = performance.now();

    for (const test of tests) {
      // Give the browser a chance to repaint between long blocking tests.
      await new Promise(requestAnimationFrame);

      await runKlausTest(qe, test);
    }

    const allElapsedMs = performance.now() - allStartedAt;

    print("=".repeat(72));
    print("ALL TESTS PASSED");
    print(`Total elapsed: ${allElapsedMs.toFixed(2)} ms`);
    print("=".repeat(72));
  }

  main().catch((error) => {
    output.textContent += "\nERROR:\n";
    output.textContent += error instanceof Error ? error.stack : String(error);
  });
}
