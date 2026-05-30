set(target netlist_lockstep)

add_executable(${target}
    main.cpp
    ${PROJECT_SOURCE_DIR}/binds/cpp/src/cpu.cpp
)

target_link_libraries(${target}
    PRIVATE
        qe6502::static
        perfect6502
)

target_include_directories(${target}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/binds/cpp/include
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

target_compile_features(${target} PUBLIC cxx_std_17)

add_test(NAME qe6502.v2.netlist_lockstep
    COMMAND ${target} 10 42
)

set_tests_properties(qe6502.v2.netlist_lockstep
    PROPERTIES
        LABELS "cpu;v2;netlist;lockstep"
        TIMEOUT 20
)
