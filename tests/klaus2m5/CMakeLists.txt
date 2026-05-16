
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
    PRIVATE qe6502
)

if(WIN32 AND QE6502_BUILD_SHARED)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:qe6502>
            $<TARGET_FILE_DIR:${target}>
        COMMENT "Copying qe6502 runtime DLL for klaus2m5"
    )
endif()

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

if(QE6502_ENABLE_NMOS_6502)
    add_test(NAME qe6502.klaus2m5.mos.standard
        COMMAND ${target} mos standard
    )
    set_tests_properties(
        qe6502.klaus2m5.mos.standard
        PROPERTIES
            LABELS "cpu;klaus;functional"
            TIMEOUT 10
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
        TIMEOUT 20
)

endif()
