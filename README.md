# qe6502

`qe6502` is a small, embeddable 6502/65C02 CPU emulator core built around an explicit external bus interface. The host owns memory and devices, services one bus request at a time, and feeds read data back on the following tick.

The core focuses on deterministic bus-level behavior, low integration overhead, and stable integration surfaces for C, C++, C#, Java, Python, Rust, JavaScript/WebAssembly, and other FFI users. `qe6502` is a CPU core, not a complete machine emulator.

The fast native C API keeps the complete CPU state in a 16-byte `qe6502_t`. The stable ABI API uses a fixed 64-byte opaque context, and save/load snapshots are also fixed at 64 bytes.

`qe6502` has no hidden mutable global state and performs no memory/device I/O internally; independent CPU contexts are suitable for multithreaded use.

## CPU models

The public model constants cover:

- NMOS 6502
- NES/2A03-style 6502 behavior
- WDC 65C02
- Rockwell 65C02
- Synertek 65C02

The selected model controls the opcode matrix, bus-cycle behavior, interrupt behavior, and CPU-specific instruction behavior used by the core.

## Execution model

`qe6502` is driven one external bus phase at a time.

Each tick returned by the CPU describes the next externally visible bus request:

- the 16-bit bus address;
- the current data bus value;
- status flags such as read/write, opcode fetch, internal restart sequence, and CPU jammed state.

For write cycles, the host stores the outgoing bus byte into its memory or device map. For read cycles, the host loads a byte from its memory or device map and passes that byte into the next CPU tick.

This keeps the CPU core independent from any particular machine memory layout, mapper, peripheral model, or timing scheduler.

## NMOS/NES accuracy

The NMOS and NES models are validated against `perfect6502` and the SingleStepTests 65x02 corpus.

For all stable instructions, including illegal opcodes with stable behavior, there are currently no known mismatches in externally visible cycle-by-cycle bus behavior. Interrupt behavior is also expected to match `perfect6502` for the covered IRQ/NMI scenarios, including overlap, hijacking, lost-NMI windows, NMI edge lifetime, and arbitration details.

The only intentional differences from `perfect6502` are seven unstable illegal opcodes, for which `qe6502` follows the SingleStepTests data instead:

| Opcode | Mnemonic |
| ---: | --- |
| `0x0B` | `ANC #imm` |
| `0x2B` | `ANC #imm` |
| `0x4B` | `ALR #imm` |
| `0x6B` | `ARR #imm` |
| `0x8B` | `XAA #imm` |
| `0xAB` | `LXA #imm` |
| `0xBB` | `LAS abs,Y` |

For these specific unstable cases, the SingleStepTests data is treated as the more reliable reference.

## Interrupt pins

IRQ and NMI are exposed as external CPU pins. The caller drives those pins just like an emulated machine would drive the real processor lines.

IRQ is level-sensitive. NMI is edge-sensitive. The exact bus tick on which a pin changes state is observable and can affect the following interrupt sequence.

For the NMOS 6502 model, the interrupt path is designed to match the subtle behavior observed in `perfect6502`, including IRQ/NMI overlap, BRK/NMI hijacking, NMI edge lifetime, lost-NMI timing windows, and related arbitration cases.

```c
qe6502_irq_assert(&cpu, 1); /* assert IRQ */
qe6502_irq_assert(&cpu, 0); /* deassert IRQ */

qe6502_nmi_assert(&cpu, 1); /* assert NMI */
qe6502_nmi_assert(&cpu, 0); /* deassert NMI */
```

Keep a line asserted for as long as the emulated device drives it active.

## Which API should I use?

Use the **fast static C API** when `qe6502` is compiled directly into an emulator, test harness, game tool, or other native program.

```c
#include <qe6502/qe6502.h>
```

```cmake
target_link_libraries(my_program PRIVATE qe6502::static)
```

Use the **stable shared C ABI** when the CPU core crosses a dynamic-library boundary, plugin boundary, FFI boundary, scripting boundary, or WebAssembly-style boundary.

```c
#include <qe6502/qe6502_abi.h>
```

```cmake
target_link_libraries(my_program PRIVATE qe6502::shared)
```

Use the **C++ wrapper** when writing C++17 code and you want a small RAII-style wrapper around the static C core.

```cpp
#include <qe6502/cpu.hpp>
```

```cmake
target_link_libraries(my_program PRIVATE qe6502::cpp)
```

Use the **C# binding** when integrating the ABI-backed CPU core from .NET or Mono code.

```csharp
using Qe6502;

var cpu = new Cpu(Model.Nmos);
```

Use the **Java binding** when integrating the ABI-backed CPU core from Java 25+ via the Foreign Function & Memory API.

```java
import qe6502.Cpu;
import qe6502.Model;

try (Cpu cpu = new Cpu(Model.NMOS)) {
    cpu.jumpTo(0x8000);
}
```

Use the **Python binding** when scripting against the ABI-backed CPU core from CPython. `tick()` returns the packed bus state for low-overhead Python loops.

```python
import qe6502

cpu = qe6502.CPU(qe6502.MODEL_NMOS)
```

Use the **Rust binding** when using `qe6502` from Rust. The Cargo-managed crate builds the canonical native C core from the repository `cpu/` tree and links it statically; it does not require a separately built CMake native library. The Rust CPU object is stateful: `tick()` advances the CPU and leaves the current bus pins available through `Cpu` methods such as `is_write()`, `bus_address()`, and `bus_data()`.

```rust
use qe6502::{Cpu, Model};

let mut cpu = Cpu::new(Model::Nmos);
cpu.jump_to(0x8000);
```

Use the **JavaScript/WebAssembly binding** when running the ABI-backed CPU core from Node.js or a browser.

```js
import { loadQe6502Node, Model } from "./qe6502.js";
```

All public integration layers expose the same basic model: create a CPU context, select a CPU model, start with `restart`, then service one external bus request per tick.

## Vendoring into another CMake project

The most direct integration is to vendor this repository and add it as a subdirectory. In subproject builds, tests and install rules are off by default; the options below make that explicit.

```cmake
set(QE6502_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QE6502_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/qe6502)

target_link_libraries(my_emulator PRIVATE qe6502::static)
# or:
# target_link_libraries(my_emulator PRIVATE qe6502::cpp)
```

## Build from source

Native debug build:

```sh
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
ctest --test-dir build/debug --output-on-failure
```

Native release build:

```sh
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
ctest --test-dir build/release --output-on-failure
```

Install:

```sh
cmake --install build/release --prefix /path/to/prefix
```

Then consume it from another CMake project:

```cmake
find_package(qe6502 CONFIG REQUIRED)
target_link_libraries(my_program PRIVATE qe6502::static)
```

Component checks are also supported:

```cmake
find_package(qe6502 CONFIG REQUIRED COMPONENTS C Static ABI CXX)
```

Available installed CMake targets depend on the build options:

- `qe6502::static`
- `qe6502::shared`
- `qe6502::cpp`

The Rust binding is Cargo-managed. CMake can build and test it from this source tree, but `cmake --install` does not install a Rust crate or a CMake target for Rust consumers.

## Build options

| Option | Default | Meaning |
| --- | ---: | --- |
| `QE6502_BUILD_STATIC` | `ON` | Build the fast static C library. |
| `QE6502_BUILD_SHARED` | `ON` | Build the stable shared ABI library. |
| `QE6502_BUILD_CPP` | `ON` | Build and install the C++ wrapper. Requires `QE6502_BUILD_STATIC=ON`. |
| `QE6502_BUILD_CSHARP` | `ON` | Build the C# binding when the .NET SDK is available. |
| `QE6502_BUILD_RUST` | `ON` | Build the Cargo-managed Rust binding. The Rust build stages the canonical C core from `cpu/` and statically builds it through Cargo. |
| `QE6502_REQUIRE_RUST` | `OFF` | Fail configure if `QE6502_BUILD_RUST=ON` but Cargo is unavailable. Intended for CI. |
| `QE6502_BUILD_JAVA` | `ON` | Build the Java binding when JDK 25+ development tools are available. |
| `QE6502_REQUIRE_JAVA` | `OFF` | Fail configure if `QE6502_BUILD_JAVA=ON` but JDK 25+ is unavailable. Intended for CI. |
| `QE6502_BUILD_PYTHON` | `ON` | Build the CPython binding when Python development headers are available. |
| `QE6502_REQUIRE_PYTHON` | `OFF` | Fail configure if `QE6502_BUILD_PYTHON=ON` but Python development headers are unavailable. Intended for CI. |
| `QE6502_BUILD_TESTS` | `${BUILD_TESTING}` top-level, `OFF` as subproject | Build tests and harnesses. Turn this off for dependency/package builds. |
| `QE6502_BUILD_WASM` | `OFF` | Enable the WebAssembly build mode. |
| `QE6502_ENABLE_WERROR` | `OFF` | Treat warnings as errors. Intended for maintainer/CI builds, not package consumers. |
| `QE6502_INSTALL` | `ON` top-level, `OFF` as subproject | Install headers, libraries, CMake package files, and pkg-config files. |

For a dependency-style build without harnesses:

```sh
cmake -S . -B build/package -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF \
  -DQE6502_BUILD_TESTS=OFF \
  -DQE6502_ENABLE_WERROR=OFF
cmake --build build/package
cmake --install build/package --prefix /path/to/prefix
```

## pkg-config

When installed, `qe6502` also generates pkg-config metadata for non-CMake users:

```sh
pkg-config --cflags --libs qe6502
pkg-config --cflags --libs qe6502-abi
pkg-config --cflags --libs qe6502-cpp
```

The pkg-config files are generated only for the corresponding enabled build products.

## Minimal C example

```c
#include <stdint.h>
#include <qe6502/qe6502.h>

static uint8_t memory_read(uint16_t address);
static void memory_write(uint16_t address, uint8_t value);

int main(void)
{
    qe6502_t cpu = {0};
    cpu.model = qe6502_model_nmos;

    qe6502_tick_t tick = qe6502_restart(&cpu);
    uint8_t input = 0;

    for (;;) {
        if ((tick.status & qe6502_status_writing) != 0u) {
            memory_write(tick.address, tick.bus);
            input = 0;
        } else {
            input = memory_read(tick.address);
        }

        tick = qe6502_tick(&cpu, input);
    }
}
```

`qe6502_restart()` prepares the first bus request. On each iteration, writes store the outgoing bus byte, while reads load a byte from the caller's memory/device map and pass it to `qe6502_tick()`.

## Minimal C++ example

```cpp
#include <cstdint>
#include <qe6502/cpu.hpp>

std::uint8_t memory_read(std::uint16_t address);
void memory_write(std::uint16_t address, std::uint8_t value);

int main()
{
    qe6502::cpu cpu(qe6502::model::nmos);

    cpu.restart();
    std::uint8_t input = 0;

    for (;;) {
        if (cpu.is_write()) {
            memory_write(cpu.bus_address(), cpu.bus_data());
            input = 0;
        } else {
            input = memory_read(cpu.bus_address());
        }

        cpu.tick(input);
    }
}
```

## Minimal Rust example

```rust
use qe6502::{Cpu, Model};

fn main() {
    let mut memory = [0u8; 65_536];
    memory[0x8000] = 0xEA; // NOP

    let mut cpu = Cpu::new(Model::Nmos);
    cpu.jump_to(0x8000);

    let mut input = 0u8;

    for _ in 0..1_000_000 {
        if cpu.is_write() {
            memory[cpu.bus_address() as usize] = cpu.bus_data();
            input = 0;
        } else {
            input = memory[cpu.bus_address() as usize];
        }

        cpu.tick(input);
    }
}
```

The Rust binding stages the native C core during the Cargo build and statically links the Cargo-built native object into the Rust crate. Repository builds stage from the canonical `cpu/` source tree rather than a committed copy under `binds/rust/`. `Cpu` keeps the active CPU state and current bus pins internally; the public hot path uses `Cpu` methods directly rather than exposing a public tick object.

## Minimal JavaScript / WebAssembly example

```js
import { loadQe6502Node, Model } from "./qe6502.js";

const qe = await loadQe6502Node("./qe6502_js.wasm");
const cpu = qe.createCpu(Model.nmos);

try {
  cpu.restart();
  let input = 0;

  for (;;) {
    if (cpu.isWrite()) {
      memoryWrite(cpu.busAddress(), cpu.busData());
      input = 0;
    } else {
      input = memoryRead(cpu.busAddress());
    }

    cpu.tick(input);
  }
} finally {
  cpu.dispose();
}
```

The JavaScript wrapper exposes the same bus-driven execution model as the C and C++ APIs. In browsers, use `loadQe6502Browser()` instead of `loadQe6502Node()`. JavaScript CPU objects own a WASM-side context, so call `dispose()` when the CPU is no longer needed.

## Minimal C# example

```csharp
using Qe6502;

byte MemoryRead(ushort address) => 0;
void MemoryWrite(ushort address, byte value) {}

var cpu = new Cpu(Model.Nmos);
cpu.Restart();

for (;;)
{
    if (cpu.IsWrite)
    {
        MemoryWrite(cpu.Address, cpu.Data);
        cpu.Tick();
    }
    else
    {
        cpu.Tick(MemoryRead(cpu.Address));
    }
}
```

The C# binding uses the stable shared ABI and exposes the same cycle-by-cycle external bus model as the native APIs.

## Minimal Java example

```java
import qe6502.Cpu;
import qe6502.Model;

byte[] memory = new byte[65536];
memory[0x8000] = (byte)0xEA; // NOP

try (Cpu cpu = new Cpu(Model.NMOS)) {
    cpu.jumpTo(0x8000);

    for (int i = 0; i < 1000; ++i) {
        if (cpu.isWrite()) {
            memory[cpu.address()] = (byte)cpu.data();
            cpu.tick();
        } else {
            cpu.tick(memory[cpu.address()] & 0xff);
        }
    }
}
```

The Java binding uses Java 25+ FFM over the stable shared ABI. Development-tree runs use `--enable-native-access=ALL-UNNAMED`. Java harness output directories contain the harness jar, `qe6502-java.jar`, and the native shared library, so the harnesses can also be run directly with `java --enable-native-access=ALL-UNNAMED -jar ...`. CMake run targets and CTest entries still pass `-Dqe6502.native.path=...` explicitly.

## Minimal Python example

```python
import qe6502

memory = bytearray(65536)
cpu = qe6502.CPU(qe6502.MODEL_NMOS)

bus_state = cpu.restart()

for _ in range(1_000_000):
    address = bus_state & qe6502.TICK_ADDRESS_MASK

    if bus_state & qe6502.TICK_WRITING:
        memory[address] = bus_state >> qe6502.TICK_BUS_SHIFT
        bus_state = cpu.tick()
    else:
        bus_state = cpu.tick(memory[address])
```

The Python binding uses the stable ABI. The hot path is `tick(data) -> int`; decode the returned packed bus state with the exported `TICK_*` constants.

## Interrupt input pins

Interrupt input pins are controlled explicitly through assert/deassert calls:

```js
cpu.irqAssert(true);  // assert the IRQ line
cpu.irqAssert(false); // deassert the IRQ line

cpu.nmiAssert(true);  // assert the NMI line
cpu.nmiAssert(false); // deassert the NMI line
```

`nmiAssert()` is a pin-level API, not a one-shot pulse helper. Keep the line asserted for as long as your emulated device drives it active, then deassert it.

## Save/load snapshots

`qe6502` supports a stable fixed-size 64-byte save/load snapshot format. A snapshot captures the complete CPU state, including the current internal bus-cycle phase, so restored execution resumes deterministically rather than only restoring the visible registers.

The same snapshot format is exposed through the native C API, the stable ABI API, the C++ wrapper, the C# binding, the Java binding, the Python binding, the Rust binding, and the JavaScript/WASM binding, so snapshots can be shared between bindings that use the same snapshot format.

A small C++ wrapper example:

```cpp
qe6502::cpu cpu(qe6502::model::nmos);
cpu.restart();

qe6502::cpu_snapshot snapshot = cpu.save();

/* ... later ... */

qe6502::cpu restored(snapshot);
```

## Testing

The repository includes regression harnesses for instruction behavior, bus timing, save/load replay, C#, Rust, and JavaScript/WASM bindings, ABI surface stability, and NMOS interrupt lockstep scenarios.

The NMOS interrupt implementation is checked against `perfect6502` through lockstep tests that compare externally visible bus behavior tick by tick. These tests cover IRQ/NMI timing, I-flag windows, interrupt arbitration, hijacking cases, and lost-NMI scenarios.

The test suite is intended to be useful both as a regression suite for `qe6502` itself and as documentation for the expected external bus behavior.

Maintainer presets enable tests and warning-as-error mode:

```sh
cmake --preset debug_native
cmake --build --preset debug_native
ctest --test-dir build/debug_native --output-on-failure

cmake --preset release_native
cmake --build --preset release_native
ctest --test-dir build/release_native --output-on-failure
```

For C# harnesses, when the .NET SDK is available:

```sh
cmake --build --preset release_native --target qe6502_cs_smoke
cmake --build --preset release_native --target qe6502_cs_klaus2m5
cmake --build --preset release_native --target qe6502_cs_smoke_run
cmake --build --preset release_native --target qe6502_cs_klaus2m5_run
```

For Python harnesses, when Python development headers are available:

```sh
cmake --build --preset release_native --target qe6502_py_smoke_run
cmake --build --preset release_native --target qe6502_py_klaus2m5_run
```

For Rust harnesses, when Cargo and rustc are available:

```sh
cmake --build --preset release_native --target qe6502_rust_smoke_run
cmake --build --preset release_native --target qe6502_rust_klaus2m5_run
```

For WebAssembly/JavaScript harnesses:

```sh
cmake --preset release_wasm
cmake --build --preset release_wasm
ctest --test-dir build/release_wasm --output-on-failure
```

The browser/manual harness can run smoke tests, Klaus2m5, and SingleStepTests interactively:

```sh
cmake --preset release_wasm
cmake --build --preset release_wasm
python3 -m http.server -d build/release_wasm/harness/js_manual 8000
```

Then open `http://localhost:8000/` and choose the desired runner.

Package users should normally keep `QE6502_BUILD_TESTS=OFF` and `QE6502_ENABLE_WERROR=OFF`.

## Related project: qe6502-benchmark

The separate [`qe6502-benchmark`](https://github.com/nikolaynedelchev/qe6502-benchmark) repository is a companion comparison of 6502-family CPU cores across correctness, cycle timing, host-visible bus behavior, model coverage, performance, portability, and integration trade-offs.

It is not required in order to use `qe6502`, but it provides useful context for the design choices made here. In the supplied benchmark snapshot, `qe6502` has full SingleStep instruction, cycle-count, and bus-trace correctness across all supported CPU models, while also ranking among the faster cores in the Klaus benchmark results for both NMOS and 65C02 configurations.

The exact performance numbers in that repository should be read as measurements for its specific benchmark environment, not as universal hardware-independent speeds.

## Status

`qe6502` is pre-1.0 and under active development.

The first official public release is currently targeted for October 2026, subject to API/ABI stabilization and completion of the current correctness baseline.

The core project currently includes C, C++, stable ABI, C#, Java, Python, Rust, and JavaScript/WebAssembly integration surfaces.

## License

`qe6502` is distributed under the MIT License. See [LICENSE](LICENSE) for details.
