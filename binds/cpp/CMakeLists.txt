set(QE6502_CPP_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe6502/cpu.hpp
)

add_library(qe6502_cpp_static STATIC
    ${QE6502_CPP_PUBLIC_HEADERS}
    src/cpu.cpp
)

target_link_libraries(qe6502_cpp_static
    PUBLIC qe6502_static
)

target_include_directories(qe6502_cpp_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/cpu/src>
        $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(qe6502_cpp_static PRIVATE /W4 /WX /we4505)
else()
    target_compile_options(qe6502_cpp_static PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion -Wsign-conversion -Wshadow
        -Wdouble-promotion -Wformat=2 -Wundef
        -Wunused-function -Werror=unused-function
    )
endif()

target_compile_features(qe6502_cpp_static PUBLIC cxx_std_17)

add_library(qe6502_cpp ALIAS qe6502_cpp_static)
