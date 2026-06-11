set(QE6502_RUST_TOOL_HINTS)

foreach(QE6502_RUST_HOME_VAR CARGO_HOME RUSTUP_HOME)
    if(DEFINED ENV{${QE6502_RUST_HOME_VAR}} AND NOT "$ENV{${QE6502_RUST_HOME_VAR}}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{${QE6502_RUST_HOME_VAR}}" QE6502_RUST_HOME_PATH)
        list(APPEND QE6502_RUST_TOOL_HINTS "${QE6502_RUST_HOME_PATH}/bin")
    endif()
endforeach()

if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{HOME}" QE6502_RUST_USER_HOME)
    list(APPEND QE6502_RUST_TOOL_HINTS "${QE6502_RUST_USER_HOME}/.cargo/bin")
endif()

if(WIN32 AND DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{USERPROFILE}" QE6502_RUST_USERPROFILE)
    list(APPEND QE6502_RUST_TOOL_HINTS "${QE6502_RUST_USERPROFILE}/.cargo/bin")
endif()

list(REMOVE_DUPLICATES QE6502_RUST_TOOL_HINTS)

find_program(CARGO_EXECUTABLE
    NAMES cargo cargo.exe
    HINTS ${QE6502_RUST_TOOL_HINTS}
)

if(NOT CARGO_EXECUTABLE)
    if(QE6502_REQUIRE_RUST)
        message(FATAL_ERROR "cargo not found but QE6502_REQUIRE_RUST is ON")
    endif()
    message(STATUS "cargo not found; skipping qe6502 Rust binding")
    return()
endif()

find_program(RUSTC_EXECUTABLE
    NAMES rustc rustc.exe
    HINTS ${QE6502_RUST_TOOL_HINTS}
)

if(NOT RUSTC_EXECUTABLE)
    if(QE6502_REQUIRE_RUST)
        message(FATAL_ERROR "rustc not found but QE6502_REQUIRE_RUST is ON")
    endif()
    message(STATUS "rustc not found; skipping qe6502 Rust binding")
    return()
endif()

if(NOT TARGET qe6502_static)
    if(QE6502_REQUIRE_RUST)
        message(FATAL_ERROR "qe6502_static target is not available but QE6502_REQUIRE_RUST is ON")
    endif()
    message(STATUS "qe6502_static target is not available; skipping qe6502 Rust binding")
    return()
endif()

set(QE6502_RUST_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)
set(QE6502_RUST_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR}/target)
set(QE6502_RUST_PROFILE "$<IF:$<CONFIG:Debug>,debug,release>")

set(QE6502_RUST_NATIVE_LIBRARY_LINK_NAME "qe6502_static")

add_custom_target(qe6502_rust ALL
    COMMAND ${CMAKE_COMMAND} -E env
        "QE6502_RUST_NATIVE_LIBRARY_DIR=$<TARGET_LINKER_FILE_DIR:qe6502_static>"
        "QE6502_RUST_NATIVE_LIBRARY_LINK_NAME=${QE6502_RUST_NATIVE_LIBRARY_LINK_NAME}"
        ${CARGO_EXECUTABLE} build
            --manifest-path ${QE6502_RUST_MANIFEST}
            --target-dir ${QE6502_RUST_TARGET_DIR}
            --profile $<IF:$<CONFIG:Debug>,dev,release>
    DEPENDS qe6502_static
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building qe6502 Rust binding"
    VERBATIM
)

set(QE6502_RUST_MANIFEST ${QE6502_RUST_MANIFEST} CACHE INTERNAL "qe6502 Rust binding manifest")
set(QE6502_RUST_TARGET_DIR ${QE6502_RUST_TARGET_DIR} CACHE INTERNAL "qe6502 Rust binding target directory")
set(QE6502_RUST_NATIVE_LIBRARY_LINK_NAME ${QE6502_RUST_NATIVE_LIBRARY_LINK_NAME} CACHE INTERNAL "qe6502 Rust native library link name")
