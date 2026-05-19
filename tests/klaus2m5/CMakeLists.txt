
set(target klaus2m5)

# Public header files

# Private source files
file(GLOB_RECURSE KLAUS2M5_PRIVATE_SOURCE
        main.c
        klaus2m5.c
        test_roms.c)

# Create target
add_executable(${target}
    ${KLAUS2M5_PRIVATE_SOURCE}
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

    add_test(NAME qe6502.klaus2m5.mos.standard
        COMMAND ${target} mos standard off
    )

    # add_test(NAME qe6502.klaus2m5.mos.standard.reset_cycles
    #     COMMAND ${target} mos standard on
    # )

    set_tests_properties(
        qe6502.klaus2m5.mos.standard
        PROPERTIES
            LABELS "cpu;klaus;functional"
            TIMEOUT 30
    )

    set_tests_properties(
        qe6502.klaus2m5.mos.standard.reset_cycles
        PROPERTIES
            LABELS "cpu;klaus;functional;serialization"
            TIMEOUT 180
    )

endif()

if(QE6502_ENABLE_CMOS_65C02)

    add_test(NAME qe6502.klaus2m5.wdc.standard
        COMMAND ${target} wdc standard off
    )

    add_test(NAME qe6502.klaus2m5.rw.standard
        COMMAND ${target} rw standard off
    )

    add_test(NAME qe6502.klaus2m5.st.standard
        COMMAND ${target} st standard off
    )

    add_test(NAME qe6502.klaus2m5.wdc.extended
        COMMAND ${target} wdc extended off
    )

    # add_test(NAME qe6502.klaus2m5.wdc.extended.reset_cycles
    #     COMMAND ${target} wdc extended on
    # )

    add_test(NAME qe6502.klaus2m5.rw.extended
        COMMAND ${target} rw extended off
    )

    set_tests_properties(
        qe6502.klaus2m5.wdc.standard
        qe6502.klaus2m5.rw.standard
        qe6502.klaus2m5.st.standard
        qe6502.klaus2m5.wdc.extended
        qe6502.klaus2m5.rw.extended
        PROPERTIES
            LABELS "cpu;klaus;functional"
            TIMEOUT 30
    )

    set_tests_properties(
        qe6502.klaus2m5.wdc.extended.reset_cycles
        PROPERTIES
            LABELS "cpu;klaus;functional;serialization"
            TIMEOUT 180
    )

endif()
