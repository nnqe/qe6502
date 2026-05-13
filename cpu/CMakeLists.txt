# qe6502/CMakeLists.txt
# qe6502: no external dependencies

set(target qe6502)

# Determine library type
if(QE6502_BUILD_SHARED)
    set(QE6502_BUILD_TYPE SHARED)
else()
    set(QE6502_BUILD_TYPE STATIC)
endif()

# Public header files
set(QE6502_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe6502/qe6502.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe6502/qe6502_macros.h
)

# Private source files
file(GLOB_RECURSE QE6502_PRIVATE_SOURCE  ./src/*.c)


if(NOT QE6502_BUILD_WASM)

    # Create target
    add_library(${target} ${QE6502_BUILD_TYPE}
        ${QE6502_PUBLIC_HEADERS}
        ${QE6502_PRIVATE_SOURCE}
    )

    # Include directories
    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    # Compile definitions (propagated to users)
    target_compile_definitions(${target}
        PUBLIC
            QE6502_ENABLE_CYCLE_MERGE=$<BOOL:${QE6502_ENABLE_CYCLE_MERGE}>
            QE6502_ENABLE_NMOS_6502=$<BOOL:${QE6502_ENABLE_NMOS_6502}>
            QE6502_ENABLE_CMOS_65C02=$<BOOL:${QE6502_ENABLE_CMOS_65C02}>
        PRIVATE
            QE6502_BUILD_SHARED=$<BOOL:${QE6502_BUILD_SHARED}>
            QE6502_BUILDING_LIBRARY=$<BOOL:${QE6502_BUILD_SHARED}>
            QE6502_ENABLE_DEBUG_LOG=$<BOOL:${QE6502_ENABLE_DEBUG_LOG}>
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /we4505)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Werror
            -Wconversion -Wsign-conversion -Wshadow
            -Wdouble-promotion -Wformat=2 -Wundef
            -Wunused-function -Werror=unused-function
        )
    endif()

    # Set C standard
    target_compile_features(${target} PUBLIC c_std_11)
else()

    set(wasm_target qe6502_wasm)

    add_executable(${wasm_target}
        ${QE6502_PUBLIC_HEADERS}
        ${QE6502_PRIVATE_SOURCE}
        #${CMAKE_CURRENT_SOURCE_DIR}/wasm/qe6502_wasm_alloc.c
    )

    target_include_directories(${wasm_target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )

    target_compile_definitions(${wasm_target}
        PUBLIC
            QE6502_ENABLE_CYCLE_MERGE=$<BOOL:${QE6502_ENABLE_CYCLE_MERGE}>
            QE6502_ENABLE_NMOS_6502=$<BOOL:${QE6502_ENABLE_NMOS_6502}>
            QE6502_ENABLE_CMOS_65C02=$<BOOL:${QE6502_ENABLE_CMOS_65C02}>
        PRIVATE
            QE6502_BUILD_SHARED=0
            QE6502_BUILDING_LIBRARY=1
            QE6502_ENABLE_DEBUG_LOG=$<BOOL:${QE6502_ENABLE_DEBUG_LOG}>
    )

    target_compile_features(${wasm_target} PUBLIC c_std_11)

    target_compile_options(${wasm_target} PRIVATE
        -O3
        -ffreestanding
        -fno-builtin
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion -Wsign-conversion -Wshadow
        -Wdouble-promotion -Wformat=2 -Wundef
        -Wunused-function -Werror=unused-function
    )

    target_link_options(${wasm_target} PRIVATE
        -nostdlib
        -Wl,--no-entry
        -Wl,--export-all
    )

    set_target_properties(${wasm_target} PROPERTIES
        OUTPUT_NAME "qe6502"
        SUFFIX ".wasm"
    )
endif()
