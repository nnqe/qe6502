# qe6502 JavaScript / WebAssembly binding

`qe6502` is a cycle-oriented 6502 / 65C02 CPU emulator. This package provides the JavaScript API and bundled WebAssembly module for Node.js and browser consumers.

## Requirements

- Node.js 22 or newer for Node.js consumers.
- A modern browser with WebAssembly support for browser consumers.

## Install

```sh
npm install qe6502
```

## Node.js use

The Node.js loader can find the bundled `qe6502_js.wasm` file automatically:

```js
import { loadQe6502Node, Model } from "qe6502";

const qe = await loadQe6502Node();
const cpu = qe.createCpu(Model.nmos);
```

You can also pass an explicit WebAssembly file path or URL when embedding qe6502 in a custom environment:

```js
const qe = await loadQe6502Node("/custom/path/qe6502_js.wasm");
```

## Browser use

Browser consumers should pass the WebAssembly module URL explicitly. This keeps the package usable with plain static hosting, bundlers, CDNs, and asset pipelines that rename or relocate `.wasm` files.

```js
import { loadQe6502Browser, Model } from "qe6502";

const qe = await loadQe6502Browser("/assets/qe6502_js.wasm");
const cpu = qe.createCpu(Model.nmos);
```

## Minimal CPU loop

`qe6502` is bus-cycle oriented. The caller observes the current bus request and supplies memory data on read cycles. On write cycles, the caller stores the CPU data byte.

```js
import { loadQe6502Node, Model } from "qe6502";

const qe = await loadQe6502Node();
const cpu = qe.createCpu(Model.nmos);
const memory = new Uint8Array(65536);

cpu.jumpTo(0x8000);

for (let i = 0; i < 1_000_000; ++i) {
  if (cpu.isWrite()) {
    memory[cpu.busAddress()] = cpu.busData();
    cpu.tick();
  } else {
    cpu.tick(memory[cpu.busAddress()]);
  }
}

cpu.dispose();
```

## CPU models

```js
const cpu = qe.createCpu(Model.nmos);
```

Available models:

- `Model.nmos` - NMOS 6502
- `Model.nes` - NES / RP2A03 CPU
- `Model.wdc` - WDC 65C02
- `Model.rw` - Rockwell 65C02
- `Model.st` - ST 65C02

## Bus state

After `jumpTo`, `restart`, `load`, or `tick`, the CPU exposes the last native bus tick:

```js
cpu.busAddress();
cpu.busData();
cpu.busStatus();
cpu.isWrite();
cpu.isOpcodeFetch();
cpu.isInternalReset();
cpu.isJammed();
```

## Registers and flags

The wrapper exposes CPU registers:

```js
cpu.pc();
cpu.setPc(0x8000);
cpu.s();
cpu.setS(0xff);
cpu.a();
cpu.setA(0x42);
cpu.x();
cpu.setX(0x00);
cpu.y();
cpu.setY(0x00);
cpu.p();
cpu.setP(0x24);
```

Status flag constants are available through `Flag`:

```js
import { Flag } from "qe6502";

cpu.setP(cpu.p() | Flag.i);
```

## Interrupt inputs

Use `nmiAssert` and `irqAssert` to control the logical interrupt input state:

```js
cpu.nmiAssert(true);
cpu.tick(memory[cpu.busAddress()]);
cpu.nmiAssert(false);
```

```js
cpu.irqAssert(true);
cpu.tick(memory[cpu.busAddress()]);
cpu.irqAssert(false);
```

## Save and load

`save` returns a portable 64-byte CPU snapshot, including the last tick:

```js
const snapshot = cpu.save();
```

Restore it with:

```js
cpu.load(snapshot);
```

## Lifetime

Each CPU object owns a native WebAssembly-side context. Call `dispose()` when a CPU is no longer needed:

```js
cpu.dispose();
```

## License

MIT
