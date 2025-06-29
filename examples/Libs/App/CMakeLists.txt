# examples/CMakeLists.txt
# C++ App helper for opening windows and event loops

# Create library
add_library(App STATIC
    App.cpp
)

target_include_directories(App
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
)

#find_package(OpenGL REQUIRED)

target_link_libraries(App PUBLIC
    fmt
    Utils
    SDL2::SDL2
    imgui)

if(UNIX AND NOT APPLE)
    target_link_libraries(App PUBLIC dl pthread)
endif()

# Set C++ standard
target_compile_features(App PUBLIC cxx_std_20)
