find_program(QE6502_NODE_EXECUTABLE node)

if(QE6502_NODE_EXECUTABLE)
    add_test(
        NAME qe6502_js_smoke
        COMMAND
            "${QE6502_NODE_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/js_smoke.mjs"
            "$<TARGET_FILE:qe6502_js>"
    )

    set_tests_properties(qe6502_js_smoke PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
else()
    message(STATUS "Skipping qe6502_js_smoke: node executable not found")
endif()
