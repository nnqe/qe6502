# binds/js/CMakeLists.txt

set(QE6502_JS_BIND_SOURCE_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/qe6502.js"
    CACHE INTERNAL "Path to QE6502 JS binding source file"
)

add_executable(qe6502_js
    qe6502_js.c
)

target_compile_features(qe6502_js PRIVATE c_std_11)

target_compile_options(qe6502_js PRIVATE
    -ffreestanding
    -fno-builtin
    -Wall -Wextra -Wpedantic -Werror
    -Wconversion -Wsign-conversion -Wshadow
    -Wdouble-promotion -Wformat=2 -Wundef
    -Wunused-function -Werror=unused-function
)

target_link_libraries(qe6502_js
    PRIVATE
        -Wl,--whole-archive
        qe6502_static
        -Wl,--no-whole-archive
)

target_link_options(qe6502_js PRIVATE
    -nostdlib
    -Wl,--no-entry
    -Wl,--export-dynamic
)

set_target_properties(qe6502_js PROPERTIES
    OUTPUT_NAME "qe6502_js"
    SUFFIX ".wasm"
)
