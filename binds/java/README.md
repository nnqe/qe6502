# qe6502 Java binding

The Java binding uses the Java 25 Foreign Function & Memory API to call the
stable `qe6502_abi` shared library. It intentionally keeps the public Java API
close to the C# binding while using conventional Java method names.

## Requirements

- JDK 25 or newer, with `java`, `javac`, and `jar`
- A native qe6502 build with `QE6502_BUILD_SHARED=ON`

The CMake build looks for Java 25 automatically. On macOS it checks registered
JDK bundles and common Homebrew locations such as `/opt/homebrew/opt/openjdk@25`
and `/usr/local/opt/openjdk@25`.

## Build

```sh
cmake --preset debug_native
cmake --build --preset debug_native --target qe6502_java
```

The build creates `qe6502-java.jar` and copies the native shared library next to
it in the Java binding build output directory.

## Runtime

The binding is classpath-based in this development tree. Because the FFM API is
native access, run Java code with:

```sh
--enable-native-access=ALL-UNNAMED
```

Development harnesses pass the native library path explicitly via:

```sh
-Dqe6502.native.path=/absolute/path/to/libqe6502.dylib
```

or the platform equivalent.
