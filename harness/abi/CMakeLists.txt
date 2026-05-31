set(surface_target qe6502_abi_surface)

add_executable(${surface_target}
    surface.c
)

if(MSVC)
    target_compile_options(${surface_target} PRIVATE /W4 /WX /we4505)
else()
    target_compile_options(${surface_target} PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion -Wsign-conversion -Wshadow
        -Wdouble-promotion -Wformat=2 -Wundef
        -Wunused-function -Werror=unused-function
    )
endif()

target_compile_features(${surface_target} PUBLIC c_std_11)

target_link_libraries(${surface_target}
    PRIVATE ${CMAKE_DL_LIBS}
)

add_dependencies(${surface_target} qe6502_shared)

add_test(NAME qe6502.abi.surface
    COMMAND ${surface_target} $<TARGET_FILE:qe6502_shared>
)

set(snapshot_target qe6502_abi_snapshot)

add_executable(${snapshot_target}
    snapshot.c
)

target_link_libraries(${snapshot_target}
    PRIVATE qe6502::static
)

if(MSVC)
    target_compile_options(${snapshot_target} PRIVATE /W4 /WX /we4505)
else()
    target_compile_options(${snapshot_target} PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion -Wsign-conversion -Wshadow
        -Wdouble-promotion -Wformat=2 -Wundef
        -Wunused-function -Werror=unused-function
    )
endif()

target_compile_features(${snapshot_target} PUBLIC c_std_11)

add_test(NAME qe6502.abi.snapshot
    COMMAND ${snapshot_target}
)
