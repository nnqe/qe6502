# qe6502/CMakeLists.txt
# qe6502: no external dependencies

set(QE6502_V2_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe6502/qe6502_abi.h
)

file(GLOB_RECURSE QE6502_V2_PRIVATE_SOURCE ./src/*.c)

function(qe6502_v2_configure_core_target target)
    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_compile_definitions(${target}
        PUBLIC
            QE6502_VERBOS=$<BOOL:${QE6502_VERBOS}>
        PRIVATE
            QE_BUILD_WASM=$<BOOL:${QE_BUILD_WASM}>
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

    target_compile_features(${target} PUBLIC c_std_11)
endfunction()

if(NOT QE_BUILD_WASM)
    message(STATUS "QE6502 2 Native Build")
else()
    message(STATUS "QE6502 2 WASM Build")
endif()

add_library(qe6502_v2_static STATIC
    ${QE6502_V2_PUBLIC_HEADERS}
    ${QE6502_V2_PRIVATE_SOURCE}
)

qe6502_v2_configure_core_target(qe6502_v2_static)

target_compile_definitions(qe6502_v2_static
    PRIVATE
        QE_BUILD_SHARED=0
        QE_BUILDING_LIBRARY=1
)

set_target_properties(qe6502_v2_static PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET hidden
)

add_library(qe6502_v2 ALIAS qe6502_v2_static)

if(NOT QE_BUILD_WASM)
    add_library(qe6502_v2_shared SHARED
        ${QE6502_V2_PUBLIC_HEADERS}
        ${QE6502_V2_PRIVATE_SOURCE}
    )

    qe6502_v2_configure_core_target(qe6502_v2_shared)

    target_compile_definitions(qe6502_v2_shared
        PRIVATE
            QE_BUILD_SHARED=1
            QE_BUILDING_LIBRARY=1
    )

    set_target_properties(qe6502_v2_shared PROPERTIES
        C_VISIBILITY_PRESET hidden
    )
endif()
