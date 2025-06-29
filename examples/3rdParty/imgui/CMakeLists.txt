add_library(imgui STATIC
    upstream/imgui.cpp
    upstream/imgui_demo.cpp
    upstream/imgui_draw.cpp
    upstream/imgui_tables.cpp
    upstream/imgui_widgets.cpp

    upstream/backends/imgui_impl_sdl2.cpp
    upstream/backends/imgui_impl_sdlrenderer2.cpp
    upstream/backends/imgui_impl_glfw.cpp
    upstream/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream/backends
)

find_package(OpenGL REQUIRED)

target_link_libraries(imgui PUBLIC SDL2::SDL2 glfw OpenGL::GL)
