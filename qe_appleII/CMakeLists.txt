# qeappleII/CMakeLists.txt
# qeappleII: depends on qe6502

set(target qeappleII)

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
add_library(${target} ${QEAPPLEII_BUILD_TYPE}
    ${QEAPPLEII_PUBLIC_HEADERS}
    ${QEAPPLEII_PRIVATE_SOURCE}
)

# Include directories
target_include_directories(${target}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependency on lib1
target_link_libraries(${target}
    PUBLIC qe6502
)

# Compile definitions (propagated to users)
target_compile_definitions(${target}
    PUBLIC
        $<$<BOOL:${QE_BUILD_SHARED}>:QE_BUILD_SHARED>
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
