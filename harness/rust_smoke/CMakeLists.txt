if(NOT QE6502_BUILD_RUST)
    message(STATUS "QE6502_BUILD_RUST is OFF; skipping qe6502 Rust smoke harness")
    return()
endif()

if(NOT CARGO_EXECUTABLE)
    message(STATUS "cargo not found; skipping qe6502 Rust smoke harness")
    return()
endif()

if(NOT TARGET qe6502_static)
    message(STATUS "qe6502_static target is not available; skipping qe6502 Rust smoke harness")
    return()
endif()

if(NOT TARGET qe6502_rust)
    message(STATUS "qe6502_rust target is not available; skipping qe6502 Rust smoke harness")
    return()
endif()

set(QE6502_RUST_SMOKE_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)
set(QE6502_RUST_SMOKE_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR}/target)
set(QE6502_RUST_SMOKE_PROFILE "$<IF:$<CONFIG:Debug>,debug,release>")
set(QE6502_RUST_SMOKE_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dist)
set(QE6502_RUST_SMOKE_EXECUTABLE
    ${QE6502_RUST_SMOKE_TARGET_DIR}/${QE6502_RUST_SMOKE_PROFILE}/qe6502_rust_smoke${CMAKE_EXECUTABLE_SUFFIX}
)

set(QE6502_RUST_SMOKE_DIST_EXECUTABLE
    ${QE6502_RUST_SMOKE_OUTPUT_DIR}/qe6502_rust_smoke${CMAKE_EXECUTABLE_SUFFIX}
)

add_custom_target(qe6502_rust_smoke ALL
    COMMAND ${CMAKE_COMMAND} -E env
        "QE6502_RUST_NATIVE_LIBRARY_DIR=$<TARGET_LINKER_FILE_DIR:qe6502_static>"
        "QE6502_RUST_NATIVE_LIBRARY_LINK_NAME=${QE6502_RUST_NATIVE_LIBRARY_LINK_NAME}"
        ${CARGO_EXECUTABLE} build
            --manifest-path ${QE6502_RUST_SMOKE_MANIFEST}
            --target-dir ${QE6502_RUST_SMOKE_TARGET_DIR}
            $<$<NOT:$<CONFIG:Debug>>:--release>
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_RUST_SMOKE_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QE6502_RUST_SMOKE_EXECUTABLE}
        ${QE6502_RUST_SMOKE_DIST_EXECUTABLE}
    DEPENDS qe6502_static qe6502_rust
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building qe6502 Rust smoke harness"
    VERBATIM
)

add_custom_target(qe6502_rust_smoke_run
    COMMAND ${QE6502_RUST_SMOKE_DIST_EXECUTABLE}
    DEPENDS qe6502_rust_smoke
    WORKING_DIRECTORY ${QE6502_RUST_SMOKE_OUTPUT_DIR}
    COMMENT "Running qe6502 Rust smoke harness"
    VERBATIM
)

add_test(NAME qe6502_rust_smoke
    COMMAND ${QE6502_RUST_SMOKE_DIST_EXECUTABLE}
    WORKING_DIRECTORY ${QE6502_RUST_SMOKE_OUTPUT_DIR}
)

set_tests_properties(qe6502_rust_smoke PROPERTIES
    LABELS "rust;smoke;functional"
    TIMEOUT 30
)
