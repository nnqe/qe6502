set(target trace)

add_executable(${target}
    main.cpp
    perfect_trace_backend.cpp
    qe_trace_backend.cpp
    trace_format.cpp
    trace_help.cpp
    trace_script.cpp
    ../third_party/perfect6502/netlist_sim.c
    ../third_party/perfect6502/perfect6502.c
)

target_include_directories(${target}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/cpu_v2/src
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(${target}
    PRIVATE
        qe6502_v2_static
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

target_compile_features(${target}
    PRIVATE
        cxx_std_17
)
