# /CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(QuckAndEasy6502 LANGUAGES C CXX)

# Public build-time options for qe6502
option(QE6502_BUILD_TESTS "Build tests" OFF)
option(QE6502_BUILD_SHARED "Enable shared build of qe6502" OFF)
option(QE6502_ENABLE_DEBUG_LOG "Enable logging (adds -DQE6502_ENABLE_DEBUG_LOG)" OFF)
option(QE6502_ENABLE_CYCLE_MERGE "Enable cpu cycles merge (adds -DQE6502_ENABLE_CYCLE_MERGE)" OFF)
option(QE6502_ENABLE_NMOS_6502 "Enable NMOS 6502 (adds -DQE6502_ENABLE_NMOS_6502)" ON)
option(QE6502_ENABLE_CMOS_65C02 "Enable CMOS 65C02 (adds -DQE6502_ENABLE_CMOS_65C02)" ON)

# Add subdirectories
add_subdirectory(qe_6502)
if(QE6502_BUILD_TESTS)
    add_subdirectory(tests)
endif()