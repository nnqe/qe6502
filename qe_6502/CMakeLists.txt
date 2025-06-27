# qe6502/CMakeLists.txt
# qe6502: no external dependencies

# Determine library type
if(QE_BUILD_SHARED)
    set(QE6502_BUILD_TYPE SHARED)
else()
    set(QE6502_BUILD_TYPE STATIC)
endif()

# Public header files
set(QE6502_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe_6502.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe_utils.h
)

# Private source files
file(GLOB_RECURSE QE6502_PRIVATE_SOURCE  ./src/*.c)


# Create target
add_library(qe6502 ${QE6502_BUILD_TYPE}
    ${QE6502_PUBLIC_HEADERS}
    ${QE6502_PRIVATE_SOURCE}
)

# Include directories
target_include_directories(qe6502
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Compile definitions (propagated to users)
target_compile_definitions(qe6502
    PUBLIC
        $<$<BOOL:${QE_BUILD_SHARED}>:QE_BUILD_SHARED>
        $<$<BOOL:${QE_ENABLE_DEBUG_LOG}>:QE_ENABLE_DEBUG_LOG>
        $<$<BOOL:${QE6502_ENABLE_CYCLE_MERGE}>:QE6502_ENABLE_CYCLE_MERGE>
        $<$<BOOL:${QE6502_ENABLE_NMOS_6502}>:QE6502_ENABLE_NMOS_6502>
        $<$<BOOL:${QE6502_ENABLE_CMOS_65C02}>:QE6502_ENABLE_CMOS_65C02>
)

# Set C standard
target_compile_features(qe6502 PUBLIC c_std_99)
