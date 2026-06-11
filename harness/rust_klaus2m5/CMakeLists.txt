if(NOT QE6502_BUILD_RUST)
    message(STATUS "QE6502_BUILD_RUST is OFF; skipping qe6502 Rust Klaus2m5 harness")
    return()
endif()

if(NOT CARGO_EXECUTABLE)
    message(STATUS "cargo not found; skipping qe6502 Rust Klaus2m5 harness")
    return()
endif()

if(NOT TARGET qe6502_static)
    message(STATUS "qe6502_static target is not available; skipping qe6502 Rust Klaus2m5 harness")
    return()
endif()

if(NOT TARGET qe6502_rust)
    message(STATUS "qe6502_rust target is not available; skipping qe6502 Rust Klaus2m5 harness")
    return()
endif()

set(QE6502_RUST_KLAUS2M5_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)
set(QE6502_RUST_KLAUS2M5_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR}/target)
set(QE6502_RUST_KLAUS2M5_PROFILE "$<IF:$<CONFIG:Debug>,debug,release>")
set(QE6502_RUST_KLAUS2M5_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dist)
set(QE6502_RUST_KLAUS2M5_ROM_OUTPUT_DIR ${QE6502_RUST_KLAUS2M5_OUTPUT_DIR}/klaus2m5)
set(QE6502_RUST_KLAUS2M5_EXECUTABLE
    ${QE6502_RUST_KLAUS2M5_TARGET_DIR}/${QE6502_RUST_KLAUS2M5_PROFILE}/qe6502_rust_klaus2m5${CMAKE_EXECUTABLE_SUFFIX}
)
set(QE6502_RUST_KLAUS2M5_DIST_EXECUTABLE
    ${QE6502_RUST_KLAUS2M5_OUTPUT_DIR}/qe6502_rust_klaus2m5${CMAKE_EXECUTABLE_SUFFIX}
)

set(QE6502_RUST_KLAUS2M5_STANDARD_ROM
    ${CMAKE_SOURCE_DIR}/harness/klaus2m5/6502_functional_test.hex
)
set(QE6502_RUST_KLAUS2M5_EXTENDED_ROM
    ${CMAKE_SOURCE_DIR}/harness/klaus2m5/65C02_extended_opcodes_test.hex
)

add_custom_target(qe6502_rust_klaus2m5 ALL
    COMMAND ${CMAKE_COMMAND} -E env
        "QE6502_RUST_NATIVE_LIBRARY_DIR=$<TARGET_LINKER_FILE_DIR:qe6502_static>"
        "QE6502_RUST_NATIVE_LIBRARY_LINK_NAME=${QE6502_RUST_NATIVE_LIBRARY_LINK_NAME}"
        ${CARGO_EXECUTABLE} build
            --manifest-path ${QE6502_RUST_KLAUS2M5_MANIFEST}
            --target-dir ${QE6502_RUST_KLAUS2M5_TARGET_DIR}
            --profile $<IF:$<CONFIG:Debug>,dev,release>
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_RUST_KLAUS2M5_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QE6502_RUST_KLAUS2M5_EXECUTABLE}
        ${QE6502_RUST_KLAUS2M5_DIST_EXECUTABLE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_RUST_KLAUS2M5_ROM_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QE6502_RUST_KLAUS2M5_STANDARD_ROM}
        ${QE6502_RUST_KLAUS2M5_ROM_OUTPUT_DIR}/6502_functional_test.hex
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QE6502_RUST_KLAUS2M5_EXTENDED_ROM}
        ${QE6502_RUST_KLAUS2M5_ROM_OUTPUT_DIR}/65C02_extended_opcodes_test.hex
    DEPENDS
        qe6502_static
        qe6502_rust
        ${QE6502_RUST_KLAUS2M5_STANDARD_ROM}
        ${QE6502_RUST_KLAUS2M5_EXTENDED_ROM}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building qe6502 Rust Klaus2m5 harness"
    VERBATIM
)

add_custom_target(qe6502_rust_klaus2m5_run
    COMMAND ${QE6502_RUST_KLAUS2M5_DIST_EXECUTABLE}
    DEPENDS qe6502_rust_klaus2m5
    WORKING_DIRECTORY ${QE6502_RUST_KLAUS2M5_OUTPUT_DIR}
    COMMENT "Running qe6502 Rust Klaus2m5 harness"
    VERBATIM
)

set(QE6502_RUST_KLAUS2M5_TESTS
    nmos standard
    wdc standard
    wdc extended
    rw standard
    rw extended
    st standard
)

list(LENGTH QE6502_RUST_KLAUS2M5_TESTS QE6502_RUST_KLAUS2M5_TEST_COUNT)
math(EXPR QE6502_RUST_KLAUS2M5_LAST_INDEX "${QE6502_RUST_KLAUS2M5_TEST_COUNT} - 1")

foreach(index RANGE 0 ${QE6502_RUST_KLAUS2M5_LAST_INDEX} 2)
    list(GET QE6502_RUST_KLAUS2M5_TESTS ${index} model)
    math(EXPR test_index "${index} + 1")
    list(GET QE6502_RUST_KLAUS2M5_TESTS ${test_index} test_name)

    add_test(NAME qe6502.rust.klaus2m5.${model}.${test_name}
        COMMAND ${QE6502_RUST_KLAUS2M5_DIST_EXECUTABLE} ${model} ${test_name}
        WORKING_DIRECTORY ${QE6502_RUST_KLAUS2M5_OUTPUT_DIR}
    )

    set_tests_properties(qe6502.rust.klaus2m5.${model}.${test_name} PROPERTIES
        LABELS "rust;v2;klaus;functional;performance"
        TIMEOUT 180
    )
endforeach()
