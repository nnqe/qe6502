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

The build creates `qe6502-java.jar`, embeds the current platform native shared
library inside it under `qe6502/native/<platform>/`, and also copies the native
shared library next to it in the Java binding build output directory for local
fallback/debug use.

## Runtime

The binding is classpath-based in this development tree. Because the FFM API is
native access, run Java code with:

```sh
--enable-native-access=ALL-UNNAMED
```

The binding first checks `-Dqe6502.native.path=/absolute/path/to/library` when it
is provided. If that property is absent, it tries to extract and load the
platform native library bundled in `qe6502-java.jar`. If no matching bundled
library is available, it tries to load the platform native library from the
current working directory before falling back to the normal system library
lookup.

Development harness output directories contain the harness jar, `qe6502-java.jar`,
and the native shared library. From a harness output directory, the smoke and
Klaus harnesses can be started with normal jar commands, for example:

```sh
java --enable-native-access=ALL-UNNAMED -jar qe6502-java-smoke.jar
java --enable-native-access=ALL-UNNAMED -jar qe6502-java-klaus2m5.jar
java --enable-native-access=ALL-UNNAMED -jar qe6502-java-klaus2m5.jar nmos standard
```
