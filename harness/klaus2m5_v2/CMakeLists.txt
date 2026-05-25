set(target klaus2m5_v2)

file(GLOB_RECURSE KLAUS2M5_V2_PRIVATE_SOURCE
        main.c
        klaus2m5_v2.c
        test_roms.c)

add_executable(${target}
    ${KLAUS2M5_V2_PRIVATE_SOURCE}
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

if(QE6502_ENABLE_NMOS_6502)

    add_test(NAME qe6502.v2.klaus2m5.nmos.standard
        COMMAND ${target} nmos standard off
    )

    set_tests_properties(
        qe6502.v2.klaus2m5.nmos.standard
        PROPERTIES
            LABELS "cpu;v2;klaus;functional"
            TIMEOUT 30
    )

endif()

if(QE6502_ENABLE_CMOS_65C02)

    add_test(NAME qe6502.v2.klaus2m5.wdc.standard
        COMMAND ${target} wdc standard off
    )

    add_test(NAME qe6502.v2.klaus2m5.wdc.extended
        COMMAND ${target} wdc extended off
    )

    add_test(NAME qe6502.v2.klaus2m5.rw.standard
        COMMAND ${target} rw standard off
    )

    add_test(NAME qe6502.v2.klaus2m5.rw.extended
        COMMAND ${target} rw extended off
    )

    set_tests_properties(
        qe6502.v2.klaus2m5.wdc.standard
        qe6502.v2.klaus2m5.wdc.extended
        qe6502.v2.klaus2m5.rw.standard
        qe6502.v2.klaus2m5.rw.extended
        PROPERTIES
            LABELS "cpu;v2;klaus;functional"
            TIMEOUT 30
    )

endif()
