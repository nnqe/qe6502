# examples/CMakeLists.txt
# C++ AppleII example emulator: depends on qe6502 and qeappleII

# Create executable
add_executable(AppleII
    AppleII.cpp
)

# Include paths (optional, if headers are consumed via target_link_libraries)
# target_include_directories(AppleII PRIVATE
#     ${CMAKE_SOURCE_DIR}/qe6502/include
#     ${CMAKE_SOURCE_DIR}/qeappleII/include
# )

# Link libraries
target_link_libraries(AppleII
    PRIVATE
        qeappleII
        fmt::fmt
        common
        qe6502
        appleII
        imgui::imgui
        glfw
        GLEW::GLEW
        OpenGL::GL
)
#        miniaudio

if(UNIX AND NOT APPLE)
    target_link_libraries(AppleII PRIVATE X11 dl pthread)
endif()

# Set C++ standard
target_compile_features(sample_app PUBLIC cxx_std_20)

