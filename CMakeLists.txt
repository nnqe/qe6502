# /CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(qe6502 LANGUAGES C)

# Public build-time options for qe6502
option(QE6502_BUILD_TESTS "Build tests" ON)
option(QE6502_BUILD_SHARED "Enable shared build of qe6502" OFF)
option(QE6502_BUILD_WASM "Build WebAssembly module" OFF)
option(QE6502_ENABLE_DEBUG_LOG "Enable logging (adds -DQE6502_ENABLE_DEBUG_LOG)" OFF)
option(QE6502_ENABLE_CYCLE_MERGE "Enable cpu cycles merge (adds -DQE6502_ENABLE_CYCLE_MERGE)" OFF)
option(QE6502_ENABLE_NMOS_6502 "Enable NMOS 6502 (adds -DQE6502_ENABLE_NMOS_6502)" ON)
option(QE6502_ENABLE_CMOS_65C02 "Enable CMOS 65C02 (adds -DQE6502_ENABLE_CMOS_65C02)" ON)
option(QE6502_ENABLE_MEM_ALLOC "Enable own memory allocator impl (adds -DQE6502_ENABLE_MEM_ALLOC)" OFF)
set(QE6502_MEM_ALLOC_CAPACITY "32" CACHE STRING "Memory allocator capacity in CPUs")


if(NOT QE6502_ENABLE_NMOS_6502 AND NOT QE6502_ENABLE_CMOS_65C02)
    message(FATAL_ERROR
        "Invalid qe6502 configuration: both QE6502_ENABLE_NMOS_6502 and "
        "QE6502_ENABLE_CMOS_65C02 are OFF. At least one CPU family must be enabled."
    )
endif()

if(QE6502_BUILD_WASM AND NOT QE6502_ENABLE_MEM_ALLOC)
    message(FATAL_ERROR
        "WebAssembly lib shoud be built with QE6502_ENABLE_MEM_ALLOC enabled."
    )
endif()

message(STATUS "qe6502 config:")
message(STATUS "  QE6502_ENABLE_NMOS_6502   = ${QE6502_ENABLE_NMOS_6502}")
message(STATUS "  QE6502_ENABLE_CMOS_65C02  = ${QE6502_ENABLE_CMOS_65C02}")
message(STATUS "  QE6502_ENABLE_CYCLE_MERGE = ${QE6502_ENABLE_CYCLE_MERGE}")
message(STATUS "  QE6502_BUILD_SHARED       = ${QE6502_BUILD_SHARED}")
message(STATUS "  QE6502_ENABLE_DEBUG_LOG   = ${QE6502_ENABLE_DEBUG_LOG}")
message(STATUS "  QE6502_BUILD_WASM         = ${QE6502_BUILD_WASM}")
message(STATUS "  QE6502_ENABLE_MEM_ALLOC   = ${QE6502_ENABLE_MEM_ALLOC}")
message(STATUS "  QE6502_MEM_ALLOC_CAPACITY = ${QE6502_MEM_ALLOC_CAPACITY}")


# Add subdirectories
add_subdirectory(cpu)
if(QE6502_BUILD_TESTS AND NOT QE6502_BUILD_WASM)
    add_subdirectory(tests)
endif()
