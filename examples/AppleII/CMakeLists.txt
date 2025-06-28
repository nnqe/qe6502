# examples/CMakeLists.txt
# C++ AppleII example emulator: depends on qe6502 and qeappleII

file(GLOB_RECURSE APPLEII_SOURCE_FILES CONFIGURE_DEPENDS
    ./*.cpp ./*.c
)

# Create executable
add_executable(AppleII
    ${APPLEII_SOURCE_FILES}
)

target_link_libraries(AppleII
    PRIVATE
        qeappleII
        Utils
        Audio
        App
)

# Set C++ standard
target_compile_features(AppleII PUBLIC cxx_std_20)

