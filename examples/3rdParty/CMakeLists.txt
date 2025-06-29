add_subdirectory(json)
add_subdirectory(fmt)

set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Disable Wayland backend" FORCE)
set(GLFW_BUILD_X11     ON  CACHE BOOL "Enable X11 backend"     FORCE)
add_subdirectory(glfw)

add_subdirectory(glad)
add_subdirectory(glm)
add_subdirectory(sdl)
add_subdirectory(imgui)
add_subdirectory(miniaudio)
add_subdirectory(filedialog)

