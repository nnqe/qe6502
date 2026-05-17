# /CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(qe6502 LANGUAGES C)

include(CTest)

# Public build-time options for qe6502
option(QE6502_ENABLE_CYCLE_MERGE "Enable cpu cycles merge (adds -DQE6502_ENABLE_CYCLE_MERGE)" OFF)
option(QE6502_ENABLE_NMOS_6502 "Enable NMOS 6502 (adds -DQE6502_ENABLE_NMOS_6502)" ON)
option(QE6502_ENABLE_CMOS_65C02 "Enable CMOS 65C02 (adds -DQE6502_ENABLE_CMOS_65C02)" ON)
option(QE6502_BUILD_EXAMPLES "Enable examples (adds -DQE6502_BUILD_EXAMPLES)" ON)

if(NOT QE6502_ENABLE_NMOS_6502 AND NOT QE6502_ENABLE_CMOS_65C02)
    message(FATAL_ERROR
        "Invalid qe6502 configuration: both QE6502_ENABLE_NMOS_6502 and "
        "QE6502_ENABLE_CMOS_65C02 are OFF. At least one CPU family must be enabled."
    )
endif()

message(STATUS "qe6502 config:")
message(STATUS "  QE6502_ENABLE_NMOS_6502   = ${QE6502_ENABLE_NMOS_6502}")
message(STATUS "  QE6502_ENABLE_CMOS_65C02  = ${QE6502_ENABLE_CMOS_65C02}")
message(STATUS "  QE6502_ENABLE_CYCLE_MERGE = ${QE6502_ENABLE_CYCLE_MERGE}")
message(STATUS "  QE6502_BUILD_EXAMPLES     = ${QE6502_BUILD_EXAMPLES}")


# Add subdirectories
add_subdirectory(qe)
add_subdirectory(cpu)
add_subdirectory(binds)

if(QE6502_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
