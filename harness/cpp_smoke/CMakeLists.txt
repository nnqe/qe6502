enable_language(CXX)
add_executable(qe6502_cpp_smoke
    main.cpp
)

target_link_libraries(qe6502_cpp_smoke
    PRIVATE qe6502_cpp_static
)

if(MSVC)
    target_compile_options(qe6502_cpp_smoke PRIVATE /W4 /WX)
else()
    target_compile_options(qe6502_cpp_smoke PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion -Wsign-conversion -Wshadow
        -Wdouble-promotion -Wformat=2 -Wundef
    )
endif()

target_compile_features(qe6502_cpp_smoke PRIVATE cxx_std_17)

add_test(NAME cpp_smoke COMMAND qe6502_cpp_smoke)
