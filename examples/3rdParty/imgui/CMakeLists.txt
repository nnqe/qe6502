set(target imgui)


add_library(${target} STATIC
    upstream/imgui.cpp
    upstream/imgui_demo.cpp
    upstream/imgui_draw.cpp
    upstream/imgui_tables.cpp
    upstream/imgui_widgets.cpp

    upstream/backends/imgui_impl_sdl2.cpp
    upstream/backends/imgui_impl_sdlrenderer2.cpp
)

target_include_directories(${target} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream/backends
)

find_package(OpenGL REQUIRED)

target_link_libraries(${target} PUBLIC SDL2::SDL2)
