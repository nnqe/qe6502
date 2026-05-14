if(NOT QE6502_BUILD_WASM)
    add_subdirectory(klaus2m5)
endif()

if(QE6502_BUILD_WASM)
    add_subdirectory(wasm)
endif()
