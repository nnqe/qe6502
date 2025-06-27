# qeappleII/CMakeLists.txt
# qeappleII: depends on qe6502

# Determine library type
if(QE_BUILD_SHARED)
    set(QEAPPLEII_BUILD_TYPE SHARED)
else()
    set(QEAPPLEII_BUILD_TYPE STATIC)
endif()

# Public header files
set(QEAPPLEII_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe_appleII.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe_appleIIhelpers.h
)

# Private source files
file(GLOB_RECURSE QEAPPLEII_PRIVATE_SOURCE  ./src/*.c)


# Create target
add_library(qeappleII ${QEAPPLEII_BUILD_TYPE}
    ${QEAPPLEII_PUBLIC_HEADERS}
    ${QEAPPLEII_PRIVATE_SOURCE}
)

# Include directories
target_include_directories(qeappleII
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependency on lib1
target_link_libraries(qeappleII
    PUBLIC qe6502
)

# Compile definitions (propagated to users)
target_compile_definitions(qeappleII
    PUBLIC
        $<$<BOOL:${QE_BUILD_SHARED}>:QE_BUILD_SHARED>
        $<$<BOOL:${QE_ENABLE_DEBUG_LOG}>:QE_ENABLE_DEBUG_LOG>
        $<$<BOOL:${QE6502_ENABLE_CYCLE_MERGE}>:QE6502_ENABLE_CYCLE_MERGE>
        $<$<BOOL:${QE6502_ENABLE_NMOS_6502}>:QE6502_ENABLE_NMOS_6502>
        $<$<BOOL:${QE6502_ENABLE_CMOS_65C02}>:QE6502_ENABLE_CMOS_65C02>
)

# Set C standard
target_compile_features(qeappleII PUBLIC c_std_99)
