/*
 * QE6502 has been validated against the full SingleStepTests ProcessorTests
 * corpus for every supported 6502-family model. The validation covers not only
 * final CPU state, but also every recorded bus operation during all intermediate
 * instruction cycles.
 *
 * https://github.com/SingleStepTests/ProcessorTests/tree/main
 */

import {
  loadQE6502,
  QE6502_MODEL_MOS,
  QE6502_MODEL_NES,
  QE6502_MODEL_WDC,
  QE6502_MODEL_RW,
  QE6502_MODEL_ST,
} from "./qe6502.js";

import { runSingleStepCase } from "./single_step_core.js";

import {
  sourceFromLocalFile,
  sourceFromLocalFolder,
  sourceFromGithubOpcode,
  sourceFromGithubFull,
  parseOpcodeText,
  getGithubBaseUrlForModel,
} from "./single_step_sources.js";

export async function run({ output }) {
  output.textContent = "";

  const root = document.createElement("div");
  const log = document.createElement("pre");

  output.replaceWith(root);
  root.appendChild(createUi());
  root.appendChild(log);

  let qe = null;

  function print(message = "") {
    log.textContent += message + "\n";
  }

  function clearLog() {
    log.textContent = "";
  }

  function hex8(value) {
    return "0x" + (value & 0xff).toString(16).padStart(2, "0").toUpperCase();
  }

  function formatNumber(value) {
    return value.toLocaleString("en-US");
  }

  function getSelectedModelKey() {
    return document.getElementById("ss-model").value;
  }

  function shouldSkipOpcode(meta) {
    const modelKey = getSelectedModelKey();

    if ((modelKey === "mos" || modelKey === "nes") && meta.isIllegal) {
      return true;
    }

    if ((modelKey === "mos" || modelKey === "nes") && meta.isCmosExtension) {
      return true;
    }

    // WAI and STP must be skipped, test files are empty
    if (modelKey === "wdc" && (meta.opcode === 203 || meta.opcode === 219)) {
      return true;
    }

    return false;
  }

  function getSelectedModel() {
    const value = getSelectedModelKey();

    switch (value) {
      case "mos":
        return QE6502_MODEL_MOS;
      case "nes":
        return QE6502_MODEL_NES;
      case "wdc":
        return QE6502_MODEL_WDC;
      case "rw":
        return QE6502_MODEL_RW;
      case "st":
        return QE6502_MODEL_ST;
      default:
        throw new Error(`Unknown CPU model: ${value}`);
    }
  }

  function createUi() {
    const container = document.createElement("div");

    container.innerHTML = `
      <style>
        .ss-panel {
          margin-bottom: 1rem;
          padding: 1rem;
          border: 1px solid #ccc;
          border-radius: 8px;
        }

        .ss-row {
          margin: 0.5rem 0;
        }

        .ss-row label {
          display: inline-block;
          min-width: 160px;
        }

        .ss-mode-panel {
          margin: 0.75rem 0;
          padding: 0.75rem;
          background: #f7f7f7;
          border-radius: 6px;
        }

        .ss-meta {
          margin-top: 0.75rem;
          background: #111;
          color: #eee;
          padding: 0.75rem;
          overflow: auto;
          white-space: pre-wrap;
        }

        button {
          margin-right: 0.5rem;
        }

        input[type="text"] {
          max-width: 100%;
        }
      </style>

      <div class="ss-panel">
        <div class="ss-row">
          <label for="ss-model">CPU model:</label>
          <select id="ss-model">
            <option value="mos">MOS 6502</option>
            <option value="nes">NES 6502</option>
            <option value="wdc">WDC 65C02</option>
            <option value="rw">RW 65C02</option>
            <option value="st">ST 65C02</option>
          </select>
        </div>

        <div class="ss-row">
          <label for="ss-mode">Test source:</label>
          <select id="ss-mode">
            <option value="local-file">Local single opcode file</option>
            <option value="local-folder">Local full folder</option>
            <option value="github-opcode">GitHub single opcode</option>
            <option value="github-full">GitHub full suite</option>
          </select>
        </div>

        <div id="ss-local-file-panel" class="ss-mode-panel">
          <div class="ss-row">
            <label>Opcode JSON file:</label>
            <input id="ss-local-file" type="file" accept=".json,application/json">
          </div>
        </div>

        <div id="ss-local-folder-panel" class="ss-mode-panel" hidden>
          <div class="ss-row">
            <label>Opcode folder:</label>
            <input id="ss-local-folder" type="file" webkitdirectory multiple>
          </div>
        </div>

        <div id="ss-github-opcode-panel" class="ss-mode-panel" hidden>
          <div class="ss-row">
            <label for="ss-github-base-opcode">GitHub base URL:</label>
            <input id="ss-github-base-opcode" type="text" size="80">
          </div>

          <div class="ss-row">
            <label for="ss-github-opcode">Opcode hex:</label>
            <input id="ss-github-opcode" type="text" value="7d" size="4">
          </div>

          <div class="ss-row">
            <label>Opcode metadata:</label>
            <div id="ss-opcode-meta" class="ss-meta">Loading metadata...</div>
          </div>
        </div>

        <div id="ss-github-full-panel" class="ss-mode-panel" hidden>
          <div class="ss-row">
            <label for="ss-github-base-full">GitHub base URL:</label>
            <input id="ss-github-base-full" type="text" size="80">
          </div>
        </div>

        <div class="ss-row">
          <button id="ss-run">Run</button>
          <button id="ss-clear">Clear</button>
        </div>
      </div>
    `;

    return container;
  }

  const modelSelect = document.getElementById("ss-model");
  const modeSelect = document.getElementById("ss-mode");
  const runButton = document.getElementById("ss-run");
  const clearButton = document.getElementById("ss-clear");

  const githubBaseOpcodeInput = document.getElementById(
    "ss-github-base-opcode",
  );
  const githubBaseFullInput = document.getElementById("ss-github-base-full");
  const githubOpcodeInput = document.getElementById("ss-github-opcode");
  const opcodeMetaPanel = document.getElementById("ss-opcode-meta");

  function updateGithubBaseUrlsFromModel() {
    const url = getGithubBaseUrlForModel(getSelectedModelKey());

    githubBaseOpcodeInput.value = url;
    githubBaseFullInput.value = url;
  }

  function updateOpcodeMetaPanel() {
    if (!opcodeMetaPanel) {
      return;
    }

    if (!qe) {
      opcodeMetaPanel.textContent = "QE6502 is not loaded yet.";
      return;
    }

    try {
      const opcode = parseOpcodeText(githubOpcodeInput.value);
      const meta = qe.getOpcodeMeta(opcode);

      opcodeMetaPanel.textContent =
        `Opcode:          ${hex8(opcode)}\n` +
        `Name:            ${meta.name}\n` +
        `Description:     ${meta.description}\n` +
        `Addressing mode: ${meta.addrModeStr}\n` +
        `Bytes:           ${meta.bytes}\n` +
        `Addr mode id:    ${meta.addrMode}\n` +
        `CMOS extension:  ${meta.isCmosExtension ? "yes" : "no"}\n` +
        `Illegal:         ${meta.isIllegal ? "yes" : "no"}`;
    } catch (error) {
      opcodeMetaPanel.textContent =
        error instanceof Error ? error.message : String(error);
    }
  }

  function updateVisiblePanel() {
    const mode = modeSelect.value;

    document.getElementById("ss-local-file-panel").hidden =
      mode !== "local-file";
    document.getElementById("ss-local-folder-panel").hidden =
      mode !== "local-folder";
    document.getElementById("ss-github-opcode-panel").hidden =
      mode !== "github-opcode";
    document.getElementById("ss-github-full-panel").hidden =
      mode !== "github-full";

    updateOpcodeMetaPanel();
  }

  modeSelect.addEventListener("change", updateVisiblePanel);

  modelSelect.addEventListener("change", () => {
    updateGithubBaseUrlsFromModel();
    updateOpcodeMetaPanel();
  });

  githubOpcodeInput.addEventListener("input", updateOpcodeMetaPanel);
  githubOpcodeInput.addEventListener("change", updateOpcodeMetaPanel);

  clearButton.addEventListener("click", clearLog);

  updateGithubBaseUrlsFromModel();
  updateVisiblePanel();

  print("Loading QE6502 WASM...");
  qe = await loadQE6502(`./qe6502_js.wasm?v=${Date.now()}`, {
    debugLog: (topic, message) => {
      console.debug(`[QE6502:${topic}] ${message}`);
    },
  });
  print("Loaded.");
  updateOpcodeMetaPanel();

  async function getDescriptorsFromUi() {
    const mode = modeSelect.value;

    if (mode === "local-file") {
      const input = document.getElementById("ss-local-file");

      if (!input.files || input.files.length !== 1) {
        throw new Error("Choose one local opcode JSON file");
      }

      return sourceFromLocalFile(input.files[0]);
    }

    if (mode === "local-folder") {
      const input = document.getElementById("ss-local-folder");

      if (!input.files || input.files.length === 0) {
        throw new Error("Choose a local folder with opcode JSON files");
      }

      return sourceFromLocalFolder(input.files);
    }

    if (mode === "github-opcode") {
      const baseUrl = githubBaseOpcodeInput.value;
      const opcode = parseOpcodeText(githubOpcodeInput.value);

      return sourceFromGithubOpcode(baseUrl, opcode);
    }

    if (mode === "github-full") {
      const baseUrl = githubBaseFullInput.value;

      return sourceFromGithubFull(baseUrl);
    }

    throw new Error(`Unknown source mode: ${mode}`);
  }

  async function runOpcodeFile(model, descriptor) {
    const meta = qe.getOpcodeMeta(descriptor.opcode);

    if (shouldSkipOpcode(meta)) {
      return {
        passed: true,
        skipped: true,
        opcode: descriptor.opcode,
        name: descriptor.name,
        meta,
        cases: 0,
        cycles: 0,
        elapsedMs: 0,
      };
    }

    const startedAt = performance.now();

    const text = await descriptor.loadText();
    const cases = JSON.parse(text);

    if (!Array.isArray(cases)) {
      throw new Error(`${descriptor.name}: expected JSON array`);
    }

    let totalCycles = 0;

    for (let i = 0; i < cases.length; i++) {
      const result = runSingleStepCase(qe, model, cases[i]);

      if (!result.passed) {
        return {
          passed: false,
          skipped: false,
          opcode: descriptor.opcode,
          name: descriptor.name,
          meta,
          caseIndex: i,
          cases: cases.length,
          failure: result,
          elapsedMs: performance.now() - startedAt,
        };
      }

      totalCycles += result.cycles;

      if ((i & 0x3ff) === 0) {
        await new Promise(requestAnimationFrame);
      }
    }

    return {
      passed: true,
      skipped: false,
      opcode: descriptor.opcode,
      name: descriptor.name,
      meta,
      cases: cases.length,
      cycles: totalCycles,
      elapsedMs: performance.now() - startedAt,
    };
  }

  function printFailure(result) {
    const failure = result.failure;

    print("");
    print("FAIL");
    print(
      `Opcode: ${hex8(result.opcode)} ${result.meta.name} ${result.meta.addrModeStr}`,
    );
    print(`File:   ${result.name}`);
    print(`Case:   #${result.caseIndex} ${failure.name}`);
    print(`Reason: ${failure.reason}`);

    if (failure.cycleIndex !== undefined) {
      print(`Cycle:  ${failure.cycleIndex}`);
    }

    if (failure.expected) {
      print("");
      print("Expected:");
      print(JSON.stringify(failure.expected, null, 2));
    }

    if (failure.actual) {
      print("");
      print("Actual:");
      print(JSON.stringify(failure.actual, null, 2));
    }

    if (failure.expectedRegs || failure.actualRegs) {
      print("");
      print("Registers:");
      print(
        JSON.stringify(
          {
            expected: failure.expectedRegs,
            actual: failure.actualRegs,
          },
          null,
          2,
        ),
      );
    }

    if (failure.addressHex) {
      print("");
      print(`Address:  ${failure.addressHex}`);
      print(`Expected: ${failure.expectedHex}`);
      print(`Actual:   ${failure.actualHex}`);
    }
  }

  runButton.addEventListener("click", async () => {
    runButton.disabled = true;
    clearLog();

    try {
      if (!qe) {
        throw new Error("QE6502 is not loaded");
      }

      const model = getSelectedModel();
      const descriptors = await getDescriptorsFromUi();

      print(
        `CPU model: ${modelSelect.options[modelSelect.selectedIndex].textContent}`,
      );
      print(`Loaded ${descriptors.length} opcode file descriptor(s).`);
      print("");

      let passedOpcodes = 0;
      let skippedOpcodes = 0;
      let failedOpcodes = 0;
      let totalCases = 0;
      let totalCycles = 0;

      const allStartedAt = performance.now();

      for (const descriptor of descriptors) {
        const result = await runOpcodeFile(model, descriptor);

        if (result.skipped) {
          skippedOpcodes++;
          print(`${hex8(result.opcode)} ${result.meta.name}: SKIP illegal`);
          continue;
        }

        if (!result.passed) {
          failedOpcodes++;
          printFailure(result);
          break;
        }

        passedOpcodes++;
        totalCases += result.cases;
        totalCycles += result.cycles;

        print(
          `${hex8(result.opcode)} ${result.meta.name.padEnd(4)} ` +
            `${result.meta.addrModeStr.padEnd(16)} PASS  ` +
            `cases=${formatNumber(result.cases)}  ` +
            `cycles=${formatNumber(result.cycles)}  ` +
            `elapsed=${result.elapsedMs.toFixed(2)} ms`,
        );

        await new Promise(requestAnimationFrame);
      }

      const elapsedMs = performance.now() - allStartedAt;

      print("");
      print("=".repeat(72));
      print("Summary:");
      print(`Passed opcodes:  ${formatNumber(passedOpcodes)}`);
      print(`Skipped illegal: ${formatNumber(skippedOpcodes)}`);
      print(`Failed opcodes:  ${formatNumber(failedOpcodes)}`);
      print(`Total cases:     ${formatNumber(totalCases)}`);
      print(`Total cycles:    ${formatNumber(totalCycles)}`);
      print(`Elapsed:         ${elapsedMs.toFixed(2)} ms`);

      if (failedOpcodes === 0) {
        print("");
        print("SUCCESS");
      }
    } catch (error) {
      print("");
      print("ERROR:");
      print(error instanceof Error ? error.stack : String(error));
    } finally {
      runButton.disabled = false;
    }
  });
}
