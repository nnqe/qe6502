
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
        COMMAND ${target} mos standard
    )

    set_tests_properties(
        qe6502.klaus2m5.mos.standard
        PROPERTIES
            LABELS "cpu;klaus;functional"
            TIMEOUT 120
    )

endif()

if(QE6502_ENABLE_CMOS_65C02)

    add_test(NAME qe6502.klaus2m5.wdc.standard
        COMMAND ${target} wdc standard
    )

    add_test(NAME qe6502.klaus2m5.rw.standard
        COMMAND ${target} rw standard
    )

    add_test(NAME qe6502.klaus2m5.st.standard
        COMMAND ${target} st standard
    )

    add_test(NAME qe6502.klaus2m5.wdc.extended
        COMMAND ${target} wdc extended
    )

    add_test(NAME qe6502.klaus2m5.rw.extended
        COMMAND ${target} rw extended
    )

    set_tests_properties(
        qe6502.klaus2m5.wdc.standard
        qe6502.klaus2m5.rw.standard
        qe6502.klaus2m5.st.standard
        qe6502.klaus2m5.wdc.extended
        qe6502.klaus2m5.rw.extended
        PROPERTIES
            LABELS "cpu;klaus;functional"
            TIMEOUT 120
    )

endif()
