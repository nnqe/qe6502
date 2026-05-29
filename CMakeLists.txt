# /CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(qe6502 LANGUAGES C CXX)

include(CTest)

option(DQE6502_BUILD_BIND_JS "Enable JavaScript bind build (adds -DQE6502_BUILD_BIND_JS)" OFF)
option(DQE6502_BUILD_BIND_CPP "Enable CPP bind build (adds -DQE6502_BUILD_BIND_CPP)" OFF)
option(DQE6502_BUILD_BIND_PYTHON "Enable Pyhton bind build (adds -DQE6502_BUILD_BIND_PYTHON)" OFF)

message(STATUS "  DQE6502_BUILD_BIND_JS     = ${DQE6502_BUILD_BIND_JS}")
message(STATUS "  DQE6502_BUILD_BIND_CPP     = ${DQE6502_BUILD_BIND_CPP}")
message(STATUS "  DQE6502_BUILD_BIND_PYTHON     = ${DQE6502_BUILD_BIND_PYTHON}")

# Add subdirectories
add_subdirectory(cpu)
add_subdirectory(binds)
add_subdirectory(harness)
