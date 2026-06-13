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

A package-style staging directory for the current platform can be generated with:

```sh
cmake --build --preset debug_native --target qe6502_java_package_stage
```

The staged directory is created under the Java binding build tree as
`package/qe6502-java-<version>/` and contains `qe6502-java.jar`,
`qe6502-java-sources.jar`, `qe6502-java-javadoc.jar`, `pom.xml`, this README,
and the project license. The jar in that directory is the same
embedded-native jar built by `qe6502_java`; the POM uses Maven coordinates
`io.github.nnqe:qe6502:<version>` with the version generated from the qe6502
CMake package version. Multi-platform aggregation is handled by later packaging
steps, not by this staging target.

A native runtime asset fragment for CI/package aggregation can be staged with:

```sh
cmake --build --preset debug_native --target qe6502_java_stage_runtime_asset
```

The runtime asset fragment is created under `runtime-asset/` in the Java binding
build tree and uses the same embedded-resource layout as the jar, for example
`qe6502/native/linux-x64/libqe6502.so`. Release CI uploads this fragment from
each supported OS/architecture. The CI Java package aggregation job merges those
fragments into one multi-platform package candidate whose jar contains all
supported native libraries under `qe6502/native/<platform>/`.


The same aggregation step can also be run manually after collecting all runtime
asset fragments into a single root that contains `qe6502/native/`:

```sh
cmake \
  -DQE6502_JAVA_JAR_EXECUTABLE="$(command -v jar)" \
  -DQE6502_JAVA_BASE_PACKAGE_DIR="$PWD/build/release_native/binds/java/package/qe6502-java-<version>" \
  -DQE6502_JAVA_RUNTIME_ROOT="$PWD/build/java-multiplatform-runtime" \
  -DQE6502_JAVA_AGGREGATE_DIR="$PWD/build/java-package" \
  -DQE6502_JAVA_PACKAGE_VERSION="<version>" \
  -P binds/java/run_package_aggregate.cmake
```

The aggregate script overlays the collected native resources into the staged jar,
copies the staged sources jar, javadoc jar, and `pom.xml`, and verifies that the
six supported platform entries are present.

A Maven-style publish layout can be generated from the staged package with:

```sh
cmake --build --preset debug_native --target qe6502_java_maven_publish_layout
```

The layout is staged under `maven-publish/io/github/nnqe/qe6502/<version>/` and
contains `qe6502-<version>.jar`, `qe6502-<version>-sources.jar`,
`qe6502-<version>-javadoc.jar`, and `qe6502-<version>.pom`. This Maven publish
layout is the only Java release candidate uploaded by CI; the package staging
directory remains an internal build/smoke-test input and is not part of the final
publish artifact. If GPG is available, the companion signing target produces
detached ASCII signatures next to those files:

```sh
cmake --build --preset debug_native --target qe6502_java_maven_sign_artifacts
```

A clean external consumer smoke can be run against the staged jar with:

```sh
cmake --build --preset debug_native --target qe6502_java_package_smoke
```

The package smoke compiles a temporary Java consumer outside the source tree,
uses only the staged `qe6502-java.jar` on the classpath, and runs without
`-Dqe6502.native.path` so the embedded native-resource path is exercised. Release
CI runs this package smoke on each supported native release platform before
uploading that platform's Java runtime asset fragment.

When Maven is available, a clean Maven consumer smoke can also be run with:

```sh
cmake --build --preset debug_native --target qe6502_java_maven_package_smoke
```

The Maven smoke installs the staged jar and generated POM into a temporary local
Maven repository, builds a temporary consumer that depends on
`io.github.nnqe:qe6502:<version>`, and runs the consumer without
`-Dqe6502.native.path`.

## Runtime

The binding is classpath-based in this development tree. Because the FFM API is
native access, run Java code with:

```sh
--enable-native-access=ALL-UNNAMED
```

The binding first checks `-Dqe6502.native.path=/absolute/path/to/library` when it
is provided. If that property is absent, it tries to extract and load the
platform native library bundled in `qe6502-java.jar`. The extraction directory
name includes the ABI version and platform id. If the current OS/architecture is
unsupported or no matching bundled library is available, the error diagnostics
say so clearly and the loader then tries the platform native library from the
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
