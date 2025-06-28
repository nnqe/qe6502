# examples/CMakeLists.txt
# C++ Audio wrapper over miniaudio: depends on miniaudio

# Create library
add_library(Audio STATIC
    Audio.cpp
)

target_include_directories(Audio
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/examples/3rdParty/miniaudio

)

target_link_libraries(miniaudio PRIVATE)

if(UNIX AND NOT APPLE)
    target_link_libraries(Audio PRIVATE dl pthread)
endif()

# Set C++ standard
target_compile_features(Audio PUBLIC cxx_std_20)
