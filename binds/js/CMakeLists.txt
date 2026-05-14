set(target qe6502_js)

set(QE6502_JS_BIND_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/qe6502.js"
    CACHE INTERNAL "Path to copied QE6502 JS binding"
)

add_custom_command(
    OUTPUT "${QE6502_JS_BIND_FILE}"

    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/qe6502.js"
            "${QE6502_JS_BIND_FILE}"

    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/qe6502.js"

    COMMENT "Copying QE6502 JS binding"
)

add_custom_target(${target} ALL
    DEPENDS "${QE6502_JS_BIND_FILE}"
)