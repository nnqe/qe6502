set(target interrupt_tests)

add_executable(${target}
    main.c
    interrupt_test_util.c
    irq_tests.c
    nmi_tests.c
    combined_tests.c
)

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

target_compile_features(${target} PUBLIC c_std_11)

add_test(NAME qe6502.interrupts.all
    COMMAND ${target} all all
)

add_test(NAME qe6502.interrupts.irq
    COMMAND ${target} irq all
)

add_test(NAME qe6502.interrupts.nmi
    COMMAND ${target} nmi all
)

add_test(NAME qe6502.interrupts.combined
    COMMAND ${target} combined all
)

set_tests_properties(
    qe6502.interrupts.all
    qe6502.interrupts.irq
    qe6502.interrupts.nmi
    qe6502.interrupts.combined
    PROPERTIES
        LABELS "cpu;interrupts"
        TIMEOUT 30
)
