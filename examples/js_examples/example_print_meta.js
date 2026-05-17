import { loadQE6502 } from "./qe6502.js";

export async function run({ output }) {
  function print(message = "") {
    output.textContent += message + "\n";
  }

  function hex8(value) {
    return "0x" + (value & 0xff).toString(16).padStart(2, "0").toUpperCase();
  }

  try {
    output.textContent = "";

    const qe = await loadQE6502(`./qe6502_js.wasm?v=${Date.now()}`, {
      debugLog: (topic, message) => {
        console.debug(`[QE6502:${topic}] ${message}`);
      },
    });

    print("QE6502 opcode metadata");
    print("=".repeat(96));
    print("");

    let legalCount = 0;
    let illegalCount = 0;
    let cmosCount = 0;

    for (let opcode = 0; opcode <= 0xff; opcode++) {
      const meta = qe.getOpcodeMeta(opcode);

      if (meta.isIllegal) {
        illegalCount++;
      } else {
        legalCount++;
      }

      if (meta.isCmosExtension) {
        cmosCount++;
      }

      print(
        [
          hex8(opcode).padEnd(6),
          meta.name.padEnd(4),
          `bytes=${String(meta.bytes).padEnd(1)}`,
          `mode=${String(meta.addrMode).padEnd(2)}`,
          `modeStr=${meta.addrModeStr.padEnd(16)}`,
          `cmos=${meta.isCmosExtension ? "yes" : "no "}`,
          `illegal=${meta.isIllegal ? "yes" : "no "}`,
          meta.description,
        ].join("  "),
      );
    }

    print("");
    print("=".repeat(96));
    print(`Legal opcodes:   ${legalCount}`);
    print(`Illegal opcodes: ${illegalCount}`);
    print(`CMOS extensions: ${cmosCount}`);
    print("Done.");
  } catch (error) {
    output.textContent += "\nERROR:\n";
    output.textContent += error instanceof Error ? error.stack : String(error);
  }
}
