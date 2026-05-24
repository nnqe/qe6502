set(target speed_benchmark_v2)

file(GLOB_RECURSE SPEEDBENCHMARK_V2_PRIVATE_SOURCE speed_benchmark_v2.c)

add_executable(${target}
    ${SPEEDBENCHMARK_V2_PRIVATE_SOURCE}
)

target_link_libraries(${target}
    PRIVATE qe6502_v2_static
)

target_include_directories(${target}
    PRIVATE ${PROJECT_SOURCE_DIR}/cpu_v2/src
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
