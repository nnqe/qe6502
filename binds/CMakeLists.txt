add_subdirectory(cpp)

if(QE_BUILD_WASM)
    add_subdirectory(js)
endif()

#add_subdirectory(py)

if(QE6502_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
