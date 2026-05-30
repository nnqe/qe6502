add_library(perfect6502 STATIC
    perfect6502.c
    netlist_sim.c
)

target_include_directories(perfect6502
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(perfect6502 PUBLIC c_std_99)
