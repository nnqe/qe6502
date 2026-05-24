set(target qe6502_js_examples)

file(GLOB_RECURSE WASM_EXAMPLE_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.html"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hex"
)

add_custom_target(${target} ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${WASM_EXAMPLE_FILES}
            "${CMAKE_CURRENT_BINARY_DIR}"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:qe6502_js>"
            "${CMAKE_CURRENT_BINARY_DIR}/qe6502_js.wasm"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QE6502_JS_BIND_SOURCE_FILE}"
            "${CMAKE_CURRENT_BINARY_DIR}"

    DEPENDS qe6502_js

    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"

    COMMENT "Copying QE6502 WASM browser examples"
)
