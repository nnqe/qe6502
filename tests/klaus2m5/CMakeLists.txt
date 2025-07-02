# qeappleII/CMakeLists.txt
# qeappleII: depends on qe6502

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

# Include directories

# Link dependency on lib1
target_link_libraries(${target}
    PRIVATE qe6502
)

# Compile definitions (propagated to users)
target_compile_definitions(${target}
    PUBLIC
        $<$<BOOL:${QE_ENABLE_DEBUG_LOG}>:QE_ENABLE_DEBUG_LOG>
        $<$<BOOL:${QE6502_ENABLE_CYCLE_MERGE}>:QE6502_ENABLE_CYCLE_MERGE>
        $<$<BOOL:${QE6502_ENABLE_NMOS_6502}>:QE6502_ENABLE_NMOS_6502>
        $<$<BOOL:${QE6502_ENABLE_CMOS_65C02}>:QE6502_ENABLE_CMOS_65C02>
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
