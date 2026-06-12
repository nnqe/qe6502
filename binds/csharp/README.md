# qe6502 C# binding

This binding is a thin, cycle-by-cycle C# wrapper over the stable `qe6502_abi` native library. It does not provide a memory callback, batch runner, or hidden bus loop; the caller drives every CPU bus cycle.

## Build

The root CMake project can build the binding when the .NET SDK is available:

```sh
cmake -S . -B build
cmake --build build --target qe6502_csharp
```

The C# project uses `DllImport("libqe6502")`, so the native shared library must be named as follows:

- Windows: `libqe6502.dll`
- Linux: `libqe6502.so`
- macOS: `libqe6502.dylib`

The CMake target places the managed output and copied native shared library under the CMake build tree, not in the source tree.


## NuGet package smoke

The CMake build can also create a current-platform NuGet package with the native `qe6502` shared library bundled as a NuGet runtime asset:

```sh
cmake --build build --target qe6502_csharp_pack
```

The package contains the managed assembly plus one native runtime asset for the platform that built it, for example `runtimes/linux-x64/native/libqe6502.so`, `runtimes/osx-arm64/native/libqe6502.dylib`, or `runtimes/win-x64/native/libqe6502.dll`.

To validate the package end-to-end, build the package smoke target. It creates a temporary external .NET console app, installs the locally built package, and runs a small CPU smoke test:

```sh
cmake --build build --target qe6502_csharp_package_smoke
```

GitHub CI runs this package smoke on each native OS job, so each platform validates its own current-platform native bundle before the future multi-RID aggregation step.

For release-native builds, CMake can also stage just the native runtime asset fragment used by future multi-RID NuGet aggregation:

```sh
cmake --build build --target qe6502_csharp_stage_runtime_asset
```

The staged tree is rooted at `runtime-asset/<config>` under the C# build directory and contains only the NuGet-ready `runtimes/<rid>/native/...` layout for the current platform. GitHub CI uploads this staged runtime asset as a build artifact from each `release_native` OS job; a later aggregation step can merge those artifacts into one all-platform package.

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

## Klaus2m5 harness

When `QE6502_BUILD_TESTS` is enabled, CMake also builds a C# Klaus2m5 console harness and registers the default supported model/test matrix with CTest:

```sh
cmake --build build --target qe6502_cs_klaus2m5
ctest --test-dir build -R qe6502.cs.klaus2m5 --output-on-failure
```

The same harness can be run directly through CMake to see the full default-suite output:

```sh
cmake --build build --target qe6502_cs_klaus2m5_run
```

The harness copies the existing `harness/klaus2m5` ROM fixtures next to the executable at build time. It can also be run manually from its output directory:

```sh
dotnet Qe6502.CsKlaus2m5.dll
dotnet Qe6502.CsKlaus2m5.dll nmos standard
dotnet Qe6502.CsKlaus2m5.dll /path/to/harness/klaus2m5 rw extended
```
