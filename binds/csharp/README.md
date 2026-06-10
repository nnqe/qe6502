# qe6502 C# binding

This binding is a thin, cycle-by-cycle C# wrapper over the stable `qe6502_abi` native library. It does not provide a memory callback, batch runner, or hidden bus loop; the caller drives every CPU bus cycle.

## Build

The root CMake project can build the binding when the .NET SDK is available:

```sh
cmake -S . -B build
cmake --build build --target qe6502_csharp
```

The C# project uses `DllImport("qe6502")`, so the native shared library must be named as follows:

- Windows: `qe6502.dll`
- Linux: `libqe6502.so`
- macOS: `libqe6502.dylib`

The CMake target places the managed output and copied native shared library under the CMake build tree, not in the source tree.

## Smoke test

When `QE6502_BUILD_TESTS` is enabled, CMake also builds a small console smoke test and registers it with CTest:

```sh
cmake -S . -B build -DQE6502_BUILD_TESTS=ON
cmake --build build --target qe6502_cs_smoke
ctest --test-dir build -R qe6502_cs_smoke --output-on-failure
```

The same smoke program can be run directly through CMake to see its normal console output:

```sh
cmake --build build --target qe6502_cs_smoke_run
```

## Minimal use

```csharp
using Qe6502;

var cpu = new Cpu(Model.Nmos);
var memory = new byte[65536];

cpu.JumpTo(0x8000);

for (int i = 0; i < 1000000; ++i) {
    if (cpu.IsWrite) {
        memory[cpu.Address] = cpu.Data;
        cpu.Tick();
    } else {
        cpu.Tick(memory[cpu.Address]);
    }
}
```

Use `NmiAsserted` and `IrqAsserted` to control the logical interrupt assertion state.
