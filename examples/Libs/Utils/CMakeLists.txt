# examples/CMakeLists.txt
# C++ Utils

file(GLOB_RECURSE UTILS_SOURCE_FILES CONFIGURE_DEPENDS
    ./*.cpp ./*.c
)

# Create library
add_library(Utils STATIC
    ${UTILS_SOURCE_FILES}
)

target_include_directories(Utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/examples/3rdParty/json/include
        PRIVATE
)

target_link_libraries(Utils PUBLIC
    fmt
)

if(UNIX AND NOT APPLE)
    target_link_libraries(Utils PUBLIC dl pthread)
endif()

# Set C++ standard
target_compile_features(Utils PUBLIC cxx_std_20)
