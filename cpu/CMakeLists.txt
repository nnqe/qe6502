# qe6502/CMakeLists.txt
# qe6502: no external dependencies

# Public header files
set(QE6502_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe6502/qe6502.h
)

# Private source files
file(GLOB_RECURSE QE6502_PRIVATE_SOURCE ./src/*.c)

function(qe6502_configure_core_target target)
    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_compile_definitions(${target}
        PUBLIC
            QE6502_ENABLE_CYCLE_MERGE=$<BOOL:${QE6502_ENABLE_CYCLE_MERGE}>
            QE6502_ENABLE_NMOS_6502=$<BOOL:${QE6502_ENABLE_NMOS_6502}>
            QE6502_ENABLE_CMOS_65C02=$<BOOL:${QE6502_ENABLE_CMOS_65C02}>
        PRIVATE
            QE_DEBUG_BUILD=$<BOOL:${QE_DEBUG_BUILD}>
            QE6502_ENABLE_EXTERN_LOG=$<BOOL:${QE6502_ENABLE_EXTERN_LOG}>
            QE6502_ENABLE_MEM_ALLOC=$<BOOL:${QE6502_ENABLE_MEM_ALLOC}>
            QE6502_MEM_ALLOC_CAPACITY=${QE6502_MEM_ALLOC_CAPACITY}
    )

    target_link_libraries(${target} PUBLIC qe_static)

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

    target_compile_features(${target} PUBLIC c_std_11)
endfunction()

if(NOT QE_BUILD_WASM)
    message(STATUS "QE6502 Native Build")

    add_library(qe6502_static STATIC
        ${QE6502_PUBLIC_HEADERS}
        ${QE6502_PRIVATE_SOURCE}
    )

    add_library(qe6502_shared SHARED
        ${QE6502_PUBLIC_HEADERS}
        ${QE6502_PRIVATE_SOURCE}
    )

    qe6502_configure_core_target(qe6502_static)
    qe6502_configure_core_target(qe6502_shared)

    target_compile_definitions(qe6502_static
        PRIVATE
            QE6502_BUILD_SHARED=0
            QE6502_BUILDING_LIBRARY=0
    )

    target_compile_definitions(qe6502_shared
        PRIVATE
            QE6502_BUILD_SHARED=1
            QE6502_BUILDING_LIBRARY=1
    )

    set_target_properties(qe6502_static PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )

    add_library(qe6502 ALIAS qe6502_static)
else()
    message(STATUS "QE6502 WASM Build")

    add_library(qe6502_static STATIC
        ${QE6502_PUBLIC_HEADERS}
        ${QE6502_PRIVATE_SOURCE}
    )

    qe6502_configure_core_target(qe6502_static)

    target_compile_definitions(qe6502_static
        PRIVATE
            QE6502_BUILD_SHARED=0
            QE6502_BUILDING_LIBRARY=1
            QE_BUILD_WASM=$<BOOL:${QE_BUILD_WASM}>
    )

    add_executable(qe6502_wasm
        ${QE6502_PUBLIC_HEADERS}
        ${QE6502_PRIVATE_SOURCE}
    )

    target_include_directories(qe6502_wasm
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )

    target_link_libraries(qe6502_wasm PRIVATE qe_static)

    target_compile_definitions(qe6502_wasm
        PUBLIC
            QE6502_ENABLE_CYCLE_MERGE=$<BOOL:${QE6502_ENABLE_CYCLE_MERGE}>
            QE6502_ENABLE_NMOS_6502=$<BOOL:${QE6502_ENABLE_NMOS_6502}>
            QE6502_ENABLE_CMOS_65C02=$<BOOL:${QE6502_ENABLE_CMOS_65C02}>
        PRIVATE
            QE_BUILD_WASM=$<BOOL:${QE_BUILD_WASM}>
            QE_DEBUG_BUILD=$<BOOL:${QE_DEBUG_BUILD}>
            QE6502_BUILD_SHARED=0
            QE6502_BUILDING_LIBRARY=1
            QE6502_ENABLE_EXTERN_LOG=$<BOOL:${QE6502_ENABLE_EXTERN_LOG}>
            QE6502_ENABLE_MEM_ALLOC=$<BOOL:${QE6502_ENABLE_MEM_ALLOC}>
            QE6502_MEM_ALLOC_CAPACITY=${QE6502_MEM_ALLOC_CAPACITY}
    )

    target_compile_features(qe6502_wasm PUBLIC c_std_11)

    target_compile_options(qe6502_wasm PRIVATE
        -ffreestanding
        -fno-builtin
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion -Wsign-conversion -Wshadow
        -Wdouble-promotion -Wformat=2 -Wundef
        -Wunused-function -Werror=unused-function
    )

    target_link_options(qe6502_wasm PRIVATE
        -nostdlib
        -Wl,--no-entry
        -Wl,--export-dynamic
    )

    set_target_properties(qe6502_wasm PROPERTIES
        OUTPUT_NAME "qe6502"
        SUFFIX ".wasm"
    )

    add_executable(qe6502 ALIAS qe6502_wasm)
endif()
