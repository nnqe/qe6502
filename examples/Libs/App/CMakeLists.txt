# examples/CMakeLists.txt
# C++ App helper for opening windows and event loops

set(target App)

# Create library
add_library(App STATIC
    App.cpp
)

target_include_directories(${target}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
)

#find_package(OpenGL REQUIRED)

target_link_libraries(${target} PUBLIC
    fmt
    Utils
    SDL2::SDL2
    imgui)

if(UNIX AND NOT APPLE)
    target_link_libraries(${target} PUBLIC dl pthread)
endif()

# Set C++ standard
target_compile_features(${target} PUBLIC cxx_std_20)
