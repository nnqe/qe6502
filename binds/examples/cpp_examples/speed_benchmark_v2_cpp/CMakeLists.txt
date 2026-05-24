set(target speed_benchmark_v2_cpp)

add_executable(${target}
    speed_benchmark_v2_cpp.cpp
)

target_link_libraries(${target}
    PRIVATE qe6502_v2_cpp_static
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

target_compile_features(${target} PUBLIC cxx_std_17)
