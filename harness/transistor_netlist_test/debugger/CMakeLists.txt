set(target perfect6502_debug)

add_executable(${target}
    main.cpp
    command_loop.cpp
    illegal_opcode_investigator.cpp
    command_input.cpp
    debugger_core.cpp
    cycle_cursor.cpp
    temporary_tests.cpp
    ui_human.cpp
    ui_ai.cpp
    help_text.cpp
    setup_input.cpp
    perfect_bridge.cpp
    bootstrap.cpp
    ../third_party/perfect6502/netlist_sim.c
    ../third_party/perfect6502/perfect6502.c
)

target_link_libraries(${target}
    PRIVATE qe6502_v2_cpp_static
)

target_include_directories(${target}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
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
        c_std_11
        cxx_std_17
)

add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/singlestep_opcode_cycle_counts.csv
        $<TARGET_FILE_DIR:${target}>/singlestep_opcode_cycle_counts.csv
)
