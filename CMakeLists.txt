# /CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(qe6502 LANGUAGES C CXX)

include(CTest)

option(QE6502_BUILD_BIND_JS "Enable JavaScript bind build" OFF)
option(QE6502_BUILD_BIND_CPP "Enable CPP bind build" OFF)
option(QE6502_BUILD_BIND_PYTHON "Enable Python bind build" OFF)

message(STATUS "  QE6502_BUILD_BIND_JS     = ${QE6502_BUILD_BIND_JS}")
message(STATUS "  QE6502_BUILD_BIND_CPP    = ${QE6502_BUILD_BIND_CPP}")
message(STATUS "  QE6502_BUILD_BIND_PYTHON = ${QE6502_BUILD_BIND_PYTHON}")

# Add subdirectories
add_subdirectory(cpu)
# add_subdirectory(binds)
add_subdirectory(harness)
