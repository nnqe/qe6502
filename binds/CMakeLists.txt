add_subdirectory(cpp)

if(QE_BUILD_WASM)
    add_subdirectory(js)
endif()

#add_subdirectory(py)
