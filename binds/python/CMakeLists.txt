find_package(Python3 COMPONENTS Interpreter Development.Module)

if(NOT Python3_FOUND)
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
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(DEFINED Python3_EXTENSION_SUFFIX)
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
