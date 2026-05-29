add_subdirectory(cpp)
add_subdirectory(py)

if(QE_BUILD_WASM)
    add_subdirectory(js)
endif()
