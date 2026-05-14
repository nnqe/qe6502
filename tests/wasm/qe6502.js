export const QE6502_MODEL_MOS = 0;
export const QE6502_MODEL_NES = 1;
export const QE6502_MODEL_WDC = 2;
export const QE6502_MODEL_RW  = 3;
export const QE6502_MODEL_ST  = 4;

const DEBUG_QE6502 = true;

function debugLog(...args) {
  if (DEBUG_QE6502) {
    console.debug("[QE6502]", ...args);
  }
}

const REQUIRED_EXPORTS = [
  "memory",
  "qe6502_cpu_alloc",
  "qe6502_cpu_free",
  "qe6502_cpu_power_on",
  "qe6502_cpu_tick_ex",

  // Used once after powerOn() to initialize the cached state.
  "qe6502_ok",
  "qe6502_error_code",
  "qe6502_needs_data",
  "qe6502_has_data",
  "qe6502_read_data",
  "qe6502_address",
  "qe6502_is_instr_done",
  "qe6502_is_started",

  "qe6502_model",

  "qe6502_read_nmi_pin",
  "qe6502_nmi_hi",
  "qe6502_nmi_lo",

  "qe6502_read_irq_pin",
  "qe6502_irq_hi",
  "qe6502_irq_lo",

  "qe6502_read_regs_packed",
  "qe6502_overwrite_regs",
  "qe6502_reset_to_normal_state",
  "qe6502_opcode_meta",
];

// qe6502_cpu_tick_ex() packed state layout:
//
// bits [ 0..15]  : Memory address
// bits [16]      : Bus direction, 0 == Read request, 1 == Write request
// bits [17]      : 0 == Started | 1 == Starting
// bits [18]      : 0 == During instruction | 1 == Instruction done
// bits [19..22]  : Reserved
// bits [23]      : 0 == OK | 1 == Halted / not OK
// bits [24..31]  : Data out, valid only when bit [16] == 1

const TICK_EX_ADDRESS_MASK = 0x0000ffff;
const TICK_EX_BUS_WRITE    = 0x00010000;
const TICK_EX_STARTING     = 0x00020000;
const TICK_EX_INSTR_DONE   = 0x00040000;
const TICK_EX_NOT_OK       = 0x00800000;
const TICK_EX_DATA_SHIFT   = 24;

export async function loadQE6502(wasmUrlOrBuffer) {
  debugLog("Start loading qe6502 wasm lib...");
  let wasmBuffer;

  if (wasmUrlOrBuffer instanceof ArrayBuffer) {
    wasmBuffer = wasmUrlOrBuffer;
  } else if (ArrayBuffer.isView(wasmUrlOrBuffer)) {
    wasmBuffer = wasmUrlOrBuffer.buffer.slice(
      wasmUrlOrBuffer.byteOffset,
      wasmUrlOrBuffer.byteOffset + wasmUrlOrBuffer.byteLength
    );
  } else if (
    typeof wasmUrlOrBuffer === "string" ||
    wasmUrlOrBuffer instanceof URL
  ) {
    const response = await fetch(wasmUrlOrBuffer);

    if (!response.ok) {
      throw new Error(
        `Failed to fetch WASM module: ${response.status} ${response.statusText}`
      );
    }

    wasmBuffer = await response.arrayBuffer();
  } else {
    throw new TypeError(
      "loadQE6502 expects a URL, string, ArrayBuffer, or typed array"
    );
  }

  const { instance } = await WebAssembly.instantiate(wasmBuffer, {});

  debugLog("Checking exports...");

  const exports = instance.exports;

  for (const name of REQUIRED_EXPORTS) {
    if (name === "memory") {
      if (!(exports.memory instanceof WebAssembly.Memory)) {
        throw new Error("Missing required WASM export: memory");
      }
    } else if (typeof exports[name] !== "function") {
      throw new Error(`Missing required WASM export: ${name}`);
    }
  }

  debugLog("QE6502 created [OK]");
  return new QE6502(exports);
}

class QE6502 {
  #exports;
  #memory;
  #textDecoder = new TextDecoder("utf-8");

  constructor(exports) {
    this.#exports = exports;
    this.#memory = exports.memory;

    if (!(this.#memory instanceof WebAssembly.Memory)) {
      throw new Error("Missing required WASM export: memory");
    }
  }

  createCPU() {
    const ptr = this.#exports.qe6502_cpu_alloc();

    if (!ptr) {
      throw new Error("qe6502_cpu_alloc returned null");
    }

    return new QE6502CPU(this.#exports, ptr);
  }

  #getMemoryU8() {
   return new Uint8Array(this.#memory.buffer);
  }

  #readCString(ptr) {
   ptr = ptr >>> 0;

   if (ptr === 0) {
     return "";
   }

   const memory = this.#getMemoryU8();

   let end = ptr;

   while (end < memory.length && memory[end] !== 0) {
     end++;
   }

   if (end >= memory.length) {
     throw new Error(`Unterminated C string at pointer ${ptr}`);
   }

   return this.#textDecoder.decode(memory.subarray(ptr, end));
  }

  getOpcodeMeta(opcode) {
    opcode = opcode & 0xff;

    const ptr = this.#exports.qe6502_opcode_meta(opcode);

    if (!ptr) {
      throw new Error(`qe6502_opcode_meta returned null for opcode 0x${opcode.toString(16).padStart(2, "0")}`);
    }

    const view = new DataView(this.#memory.buffer);

    /*
      qe6502_opcode_meta_t layout on wasm32:

      offset 0   const char* name
      offset 4   const char* description
      offset 8   const char* addr_mode_str
      offset 12  const void* reserved_ptr
      offset 16  uint8_t opcode
      offset 17  uint8_t bytes
      offset 18  uint8_t addr_mode
      offset 19  uint8_t is_cmos_extension
      offset 20  uint8_t reserved_data[4]
    */

    const namePtr = view.getUint32(ptr + 0, true);
    const descriptionPtr = view.getUint32(ptr + 4, true);
    const addrModeStrPtr = view.getUint32(ptr + 8, true);
    const name = this.#readCString(namePtr);
    return {
      name,
      description: this.#readCString(descriptionPtr),
      addrModeStr: this.#readCString(addrModeStrPtr),

      opcode: view.getUint8(ptr + 16),
      bytes: view.getUint8(ptr + 17),
      addrMode: view.getUint8(ptr + 18),
      isCmosExtension: view.getUint8(ptr + 19) !== 0,
      isIllegal: name === "ILL",
    };
  }

}

class QE6502CPU {
  #exports;
  #ptr;
  #disposed = false;

  #address = 0;
  #isWrite = false;
  #isStarted = false;
  #isInstrDone = false;
  #ok = false;
  #dataIn = 0;
  #dataOut = 0;

  constructor(exports, ptr) {
    this.#exports = exports;
    this.#ptr = ptr;

    // The CPU has not been powered on yet. Keep the object alive but not OK.
    this.#ok = false;
    this.#isStarted = false;
    this.#isInstrDone = true;
  }

  powerOn(model = QE6502_MODEL_MOS) {
    this.#assertAlive();

    this.#exports.qe6502_cpu_power_on(this.#ptr, model | 0);

    // Initialize the cached bus/status state after power-on.
    // This is intentionally done with the old granular API only once.
    // The hot path uses qe6502_cpu_tick_ex().
    this.#refreshStateSlow();

    this.#dataIn = 0;
  }

  tick() {
    this.#assertAlive();

    const packed = this.#exports.qe6502_cpu_tick_ex(
      this.#ptr,
      this.#dataIn & 0xff
    );

    this.#dataIn = 0;
    this.#cacheTickExState(packed >>> 0);
  }

  ok() {
    this.#assertAlive();
    return this.#ok;
  }

  errorCode() {
    this.#assertAlive();
    return this.#exports.qe6502_error_code(this.#ptr) | 0;
  }

  needsData() {
    this.#assertAlive();
    return !this.#isWrite;
  }

  hasData() {
    this.#assertAlive();
    return this.#isWrite;
  }

  feedData(byte) {
    this.#assertAlive();
    this.#dataIn = byte & 0xff;
  }

  readData() {
    this.#assertAlive();
    return this.#dataOut;
  }

  address() {
    this.#assertAlive();
    return this.#address;
  }

  isInstrDone() {
    this.#assertAlive();
    return this.#isInstrDone;
  }

  isStarted() {
    this.#assertAlive();
    return this.#isStarted;
  }

  model() {
    this.#assertAlive();
    return this.#exports.qe6502_model(this.#ptr) | 0;
  }

  readNmiPin() {
    this.#assertAlive();
    return this.#exports.qe6502_read_nmi_pin(this.#ptr) !== 0;
  }

  nmiHi() {
    this.#assertAlive();
    this.#exports.qe6502_nmi_hi(this.#ptr);
  }

  nmiLo() {
    this.#assertAlive();
    this.#exports.qe6502_nmi_lo(this.#ptr);
  }

  readIrqPin() {
    this.#assertAlive();
    return this.#exports.qe6502_read_irq_pin(this.#ptr) !== 0;
  }

  irqHi() {
    this.#assertAlive();
    this.#exports.qe6502_irq_hi(this.#ptr);
  }

  irqLo() {
    this.#assertAlive();
    this.#exports.qe6502_irq_lo(this.#ptr);
  }

  readRegs() {
    this.#assertAlive();

    const packed = BigInt.asUintN(
      64,
      this.#exports.qe6502_read_regs_packed(this.#ptr)
    );

    const pcl = Number(packed & 0xffn);
    const pch = Number((packed >> 8n) & 0xffn);
    const pc = pcl | (pch << 8);
    return {
      pc,
      a:      Number((packed >> 16n) & 0xffn),
      x:      Number((packed >> 24n) & 0xffn),
      y:      Number((packed >> 32n) & 0xffn),
      s:      Number((packed >> 40n) & 0xffn),
      p:      Number((packed >> 48n) & 0xffn),
      opcode: Number((packed >> 56n) & 0xffn),
    };
  }

  overwriteRegs({ pc, a, x, y, s, p, opcode = 0 }) {
    this.#assertAlive();

    const pcl = pc & 0xff;
    const pch = (pc >>> 8) & 0xff;

    const packed =
      BigInt(pcl) |
      (BigInt(pch) << 8n) |
      (BigInt(a & 0xff) << 16n) |
      (BigInt(x & 0xff) << 24n) |
      (BigInt(y & 0xff) << 32n) |
      (BigInt(s & 0xff) << 40n) |
      (BigInt(p & 0xff) << 48n) |
      (BigInt(opcode & 0xff) << 56n);

      this.#exports.qe6502_overwrite_regs(this.#ptr, packed);
      this.#exports.qe6502_reset_to_normal_state(this.#ptr);
      this.#refreshStateSlow();
  }

  dispose() {
    if (this.#disposed) {
      return;
    }

    this.#exports.qe6502_cpu_free(this.#ptr);
    this.#ptr = 0;
    this.#disposed = true;

    this.#address = 0;
    this.#isWrite = false;
    this.#isStarted = false;
    this.#isInstrDone = false;
    this.#ok = false;
    this.#dataIn = 0;
    this.#dataOut = 0;
  }

  #cacheTickExState(packed) {
    this.#address = packed & TICK_EX_ADDRESS_MASK;
    this.#isWrite = (packed & TICK_EX_BUS_WRITE) !== 0;
    this.#isStarted = (packed & TICK_EX_STARTING) === 0;
    this.#isInstrDone = (packed & TICK_EX_INSTR_DONE) !== 0;
    this.#ok = (packed & TICK_EX_NOT_OK) === 0;
    this.#dataOut = (packed >>> TICK_EX_DATA_SHIFT) & 0xff;
  }

  #refreshStateSlow() {
    this.#address = this.#exports.qe6502_address(this.#ptr) & 0xffff;

    const hasData = this.#exports.qe6502_has_data(this.#ptr) !== 0;
    const needsData = this.#exports.qe6502_needs_data(this.#ptr) !== 0;

    // The new tick_ex encoding has only read/write direction.
    // Prefer write if the CPU explicitly reports data available.
    // Otherwise treat the state as read, matching the existing bus protocol.
    this.#isWrite = hasData && !needsData;

    this.#isStarted = this.#exports.qe6502_is_started(this.#ptr) !== 0;
    this.#isInstrDone = this.#exports.qe6502_is_instr_done(this.#ptr) !== 0;
    this.#ok = this.#exports.qe6502_ok(this.#ptr) !== 0;

    this.#dataOut = this.#isWrite
      ? this.#exports.qe6502_read_data(this.#ptr) & 0xff
      : 0;
  }

  #assertAlive() {
    if (this.#disposed) {
      throw new Error("QE6502CPU has been disposed");
    }
  }
}
