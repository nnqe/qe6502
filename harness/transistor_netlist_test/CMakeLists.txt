
set(target transistor_netlist)

# Public header files

# Private source files
file(GLOB_RECURSE NETLIST_PRIVATE_SOURCE
        main.c
        netlist_helpers.c
        opcode_test.c
        opcode_sweep_test.c
        setup_test.c
        interrupt_test.c
        ./third_party/perfect6502/netlist_sim.c
        ./third_party/perfect6502/perfect6502.c)

# Create target
add_executable(${target}
    ${NETLIST_PRIVATE_SOURCE}
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

# Set C standard
target_compile_features(${target} PUBLIC c_std_11)

# Add tests
if(QE6502_ENABLE_NMOS_6502)
    add_test(NAME qe6502.netlist.mos.standard
        COMMAND ${target} mos standard
    )
    set_tests_properties(
        qe6502.netlist.mos.standard
        PROPERTIES
            LABELS "cpu;transistor_netlist;functional"
            TIMEOUT 10
    )

    add_test(NAME qe6502.netlist.mos.legal_opcodes_8
        COMMAND ${target} sweep_legal 8
    )
    set_tests_properties(
        qe6502.netlist.mos.legal_opcodes_8
        PROPERTIES
            LABELS "cpu;transistor_netlist;opcode_sweep"
            TIMEOUT 600
    )

    add_test(NAME qe6502.netlist.mos.irq_lda_abs
        COMMAND ${target} irq_lda
    )
    set_tests_properties(
        qe6502.netlist.mos.irq_lda_abs
        PROPERTIES
            LABELS "cpu;transistor_netlist;interrupt"
            TIMEOUT 60
            DISABLED TRUE
    )
endif()

add_subdirectory(debugger)
