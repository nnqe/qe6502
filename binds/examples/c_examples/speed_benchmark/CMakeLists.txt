
set(target speed_benchmark)

# Public header files

# Private source files
file(GLOB_RECURSE SPEEDBENCHMARK_PRIVATE_SOURCE speed_benchmark.c)

# Create target
add_executable(${target}
    ${SPEEDBENCHMARK_PRIVATE_SOURCE}
)

# Link dependency
target_link_libraries(${target}
    PRIVATE qe6502_static
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
