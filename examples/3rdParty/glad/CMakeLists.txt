add_library(GLAD STATIC
    ./src/gl.c
)

target_include_directories(GLAD PUBLIC
    ./include
)

target_include_directories(GLAD PUBLIC
    ${CMAKE_SOURCE_DIR}/examples/3rdParty/glad/include
)
