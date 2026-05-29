set(target klaus2m5)

file(GLOB_RECURSE KLAUS2M5_PRIVATE_SOURCE
        main.c
        klaus2m5.c
        test_roms.c)

add_executable(${target}
    ${KLAUS2M5_PRIVATE_SOURCE}
)

target_link_libraries(${target}
    PRIVATE qe6502::static
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

add_test(NAME qe6502.v2.klaus2m5.nmos.standard
    COMMAND ${target} nmos standard
)

set_tests_properties(
    qe6502.v2.klaus2m5.nmos.standard
    PROPERTIES
        LABELS "cpu;v2;klaus;functional"
        TIMEOUT 30
)

add_test(NAME qe6502.v2.klaus2m5.wdc.standard
    COMMAND ${target} wdc standard
)

add_test(NAME qe6502.v2.klaus2m5.wdc.extended
    COMMAND ${target} wdc extended
)

add_test(NAME qe6502.v2.klaus2m5.rw.standard
    COMMAND ${target} rw standard
)

add_test(NAME qe6502.v2.klaus2m5.rw.extended
    COMMAND ${target} rw extended
)

add_test(NAME qe6502.v2.klaus2m5.st.standard
    COMMAND ${target} st standard
)

set_tests_properties(
    qe6502.v2.klaus2m5.wdc.standard
    qe6502.v2.klaus2m5.wdc.extended
    qe6502.v2.klaus2m5.rw.standard
    qe6502.v2.klaus2m5.rw.extended
    qe6502.v2.klaus2m5.st.standard
    PROPERTIES
        LABELS "cpu;v2;klaus;functional"
        TIMEOUT 30
)
