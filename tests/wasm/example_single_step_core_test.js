import {
  loadQE6502,
  QE6502_MODEL_MOS,
} from "./qe6502.js";

import {
  runSingleStepCase,
} from "./single_step_core.js";

export async function run({ output }) {
  function print(message = "") {
    output.textContent += message + "\n";
  }

  function printResult(title, result) {
    print(title);
    print(JSON.stringify(result, null, 2));
    print("");
  }

  const qe = await loadQE6502(`./qe6502.wasm?v=${Date.now()}`, {
    debugLog: (topic, message) => {
      console.debug(`[QE6502:${topic}] ${message}`);
    },
  });

  /*
    Test instruction:

      $8000: A2 10    LDX #$10

    Expected cycles:

      read $8000 -> $A2
      read $8001 -> $10

    Expected final state:

      PC = $8002
      X  = $10
  */

  const correctTest = {
    name: "manual LDX #$10 correct",

    initial: {
      pc: 0x8000,
      s: 0xfd,
      a: 0x00,
      x: 0x00,
      y: 0x00,
      p: 0x30,
      ram: [
        [0x8000, 0xa2],
        [0x8001, 0x10],
        [0x8002, 0xea],
      ],
    },

    final: {
      pc: 0x8002,
      s: 0xfd,
      a: 0x00,
      x: 0x10,
      y: 0x00,
      p: 0x30,
      ram: [
        [0x8000, 0xa2],
        [0x8001, 0x10],
        [0x8002, 0xea],
      ],
    },

    cycles: [
      [0x8000, 0xa2, "read"],
      [0x8001, 0x10, "read"],
    ],
  };

  const wrongTest = structuredClone(correctTest);
  wrongTest.name = "manual LDX #$10 intentionally wrong";
  wrongTest.final.x = 0x11;

  print("Single-step core test");
  print("=".repeat(72));
  print("");

  const correctResult = runSingleStepCase(
    qe,
    QE6502_MODEL_MOS,
    correctTest
  );

  printResult("Correct test result:", correctResult);

  if (!correctResult.passed) {
    throw new Error("Expected correct test to pass");
  }

  const wrongResult = runSingleStepCase(
    qe,
    QE6502_MODEL_MOS,
    wrongTest
  );

  printResult("Wrong test result:", wrongResult);

  if (wrongResult.passed) {
    throw new Error("Expected wrong test to fail");
  }

  print("SUCCESS: correct case passed and intentionally wrong case failed.");
}
