# Toolchain for freestanding WebAssembly build using LLVM/Clang.
# Target: wasm32-unknown-unknown
#
# This does not use Emscripten or WASI.


find_program(QE6502_LLVM_AR
    NAMES llvm-ar llvm-ar-18 llvm-ar-17 llvm-ar-16 llvm-ar-15 llvm-ar-14
    REQUIRED
)

find_program(QE6502_LLVM_RANLIB
    NAMES llvm-ranlib llvm-ranlib-18 llvm-ranlib-17 llvm-ranlib-16 llvm-ranlib-15 llvm-ranlib-14
    REQUIRED
)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR wasm32)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET wasm32-unknown-unknown)

set(CMAKE_AR ${QE6502_LLVM_AR})
set(CMAKE_RANLIB ${QE6502_LLVM_RANLIB})

set(CMAKE_EXECUTABLE_SUFFIX ".wasm")

# Avoid CMake trying to link a normal executable during compiler checks.
# Freestanding Wasm has no main/_start/libc.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

