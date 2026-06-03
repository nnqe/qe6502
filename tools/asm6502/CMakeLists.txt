cmake_minimum_required(VERSION 3.16)

project(asm6502 LANGUAGES CXX)

add_library(asm6502 STATIC
    src/asm6502.cpp
)

add_library(asm6502::asm6502 ALIAS asm6502)

target_compile_features(asm6502 PUBLIC cxx_std_17)

target_include_directories(asm6502
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
