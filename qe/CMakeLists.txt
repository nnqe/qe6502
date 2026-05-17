# qe/CMakeLists.txt
# qe: no external dependencies

set(target qe_static)

# Public header files
set(QE_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe/api_public.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe/api_private.h
)

# Private source files
file(GLOB_RECURSE QE_PRIVATE_SOURCE ./src/*.c)

if(NOT QE_BUILD_WASM)
    message(STATUS "QE Native Build")
else()
    message(STATUS "QE WASM Build")
endif()

add_library(${target} STATIC
    ${QE_PUBLIC_HEADERS}
    ${QE_PRIVATE_SOURCE}
)

target_include_directories(${target}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(${target}
    PRIVATE
        QE_DEBUG_BUILD=$<BOOL:${QE_DEBUG_BUILD}>
        QE_BUILD_WASM=$<BOOL:${QE_BUILD_WASM}>
        QE_BUILD_SHARED=0
        QE_BUILDING_LIBRARY=1
)

if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /we4505)
else()
    if(NOT QE_BUILD_WASM)
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Werror
            -Wconversion -Wsign-conversion -Wshadow
            -Wdouble-promotion -Wformat=2 -Wundef
            -Wunused-function -Werror=unused-function
        )
    else()
        target_compile_options(${target} PRIVATE
            -ffreestanding
            -fno-builtin
            -Wall -Wextra -Wpedantic -Werror
            -Wconversion -Wsign-conversion -Wshadow
            -Wdouble-promotion -Wformat=2 -Wundef
            -Wunused-function -Werror=unused-function
        )
    endif()
endif()

target_compile_features(${target} PUBLIC c_std_11)

set_target_properties(${target} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_library(qe ALIAS qe_static)