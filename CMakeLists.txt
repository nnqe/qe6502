# /CMakeLists.txt (root)
cmake_minimum_required(VERSION 3.15)
project(QuckAndEasy6502 LANGUAGES C CXX)

# Public common build-time options
option(QE_BUILD_SHARED "Enable shared build of lib1" OFF)
option(QE_ENABLE_DEBUG_LOG "Enable logging (adds -DQE_ENABLE_DEBUG_LOG)" OFF)

# Public build-time options for qe6502
option(QE6502_ENABLE_CYCLE_MERGE "Enable cpu cycles merge (adds -DQE6502_ENABLE_CYCLE_MERGE)" OFF)
option(QE6502_ENABLE_NMOS_6502 "Enable NMOS 6502 (adds -DQE_ENABLE_NMOS_6502)" ON)
option(QE6502_ENABLE_CMOS_65C02 "Enable CMOS 65C02 (adds -DQE_ENABLE_CMOS_65C02)" ON)

# Public build-time options for qeappleII


# find_package(fmt REQUIRED)
# find_package(imgui CONFIG REQUIRED)
# find_package(glfw3 CONFIG REQUIRED)
# find_package(GLEW REQUIRED)
# find_package(OpenGL REQUIRED)

# Add subdirectories
add_subdirectory(qe_6502)
add_subdirectory(qe_appleII)
add_subdirectory(examples)
