# examples/CMakeLists.txt
# C++ AppleII example emulator: depends on qe6502 and qeappleII

set(target AppleII)

file(GLOB_RECURSE APPLEII_SOURCE_FILES CONFIGURE_DEPENDS
    ./*.cpp ./*.c
)

# Create executable
add_executable(${target}
    ${APPLEII_SOURCE_FILES}
)

target_compile_definitions(${target} PRIVATE SDL_MAIN_HANDLED)

target_link_libraries(${target}
    PRIVATE
        qeappleII
        Utils
        App
        SDL2::SDL2
        imgui
        portable_file_dialogs
)

# Set C++ standard
target_compile_features(${target} PUBLIC cxx_std_20)

