if(QE6502_REQUIRE_PYTHON)
    find_package(Python3 REQUIRED COMPONENTS Development.Module)
else()
    find_package(Python3 COMPONENTS Development.Module)
endif()

if(NOT Python3_Development.Module_FOUND)
    message(STATUS "Python3 development module not found; skipping qe6502 Python binding")
    return()
endif()

if(NOT TARGET qe6502_static)
    message(STATUS "qe6502_static target is not available; skipping qe6502 Python binding")
    return()
endif()

add_library(_qe6502 MODULE
    _qe6502.c
)

target_compile_features(_qe6502 PRIVATE c_std_11)
qe6502_configure_warnings(_qe6502)

target_link_libraries(_qe6502
    PRIVATE
        Python3::Module
        qe6502_static
)

set_target_properties(_qe6502 PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(WIN32)
    set_target_properties(_qe6502 PROPERTIES SUFFIX ".pyd")
elseif(DEFINED Python3_EXTENSION_SUFFIX AND NOT Python3_EXTENSION_SUFFIX STREQUAL "")
    set_target_properties(_qe6502 PROPERTIES SUFFIX "${Python3_EXTENSION_SUFFIX}")
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/qe6502/__init__.py
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/qe6502
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/qe6502/__init__.py
        ${CMAKE_CURRENT_BINARY_DIR}/qe6502/__init__.py
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/qe6502/__init__.py
    VERBATIM
)

add_custom_target(qe6502_python ALL
    DEPENDS
        _qe6502
        ${CMAKE_CURRENT_BINARY_DIR}/qe6502/__init__.py
)
