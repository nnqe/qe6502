set(target perfect6502_debug)

add_executable(${target}
    main.cpp
    perfect_bridge.cpp
    bootstrap.cpp
    ../third_party/perfect6502/netlist_sim.c
    ../third_party/perfect6502/perfect6502.c
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
