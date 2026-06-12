cmake_minimum_required(VERSION 3.15)
project(qe6502_install_consume_cpp LANGUAGES CXX)

find_package(qe6502 CONFIG REQUIRED)

if(NOT TARGET qe6502::cpp)
    message(FATAL_ERROR "Expected installed package target qe6502::cpp")
endif()

if(TARGET qe6502::cpp_static)
    message(FATAL_ERROR "Dynamic-only install must not export qe6502::cpp_static")
endif()

if(TARGET qe6502::cpp_shared)
    message(FATAL_ERROR "Dynamic-only install should expose the selected C++ target as qe6502::cpp, not qe6502::cpp_shared")
endif()

if(NOT TARGET qe6502::shared)
    message(FATAL_ERROR "Dynamic-only install must export qe6502::shared")
endif()

add_executable(qe6502_install_consume_cpp main.cpp)
target_link_libraries(qe6502_install_consume_cpp PRIVATE qe6502::cpp)
target_compile_features(qe6502_install_consume_cpp PRIVATE cxx_std_17)

if(DEFINED QE6502_INSTALL_PREFIX)
    set_target_properties(qe6502_install_consume_cpp PROPERTIES
        BUILD_RPATH "${QE6502_INSTALL_PREFIX}/lib;${QE6502_INSTALL_PREFIX}/lib64"
    )
endif()
