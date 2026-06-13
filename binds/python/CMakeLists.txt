set(QE6502_PYTHON_LIMITED_API_HEX 0x030A0000)
set(QE6502_PYTHON_STABLE_ABI_MINIMUM "3.10")

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.26)
    if(QE6502_REQUIRE_PYTHON)
        find_package(Python3 REQUIRED COMPONENTS Development.SABIModule)
    else()
        find_package(Python3 COMPONENTS Development.SABIModule)
    endif()

    if(Python3_Development.SABIModule_FOUND)
        set(QE6502_PYTHON_MODULE_TARGET Python3::SABIModule)
        set(QE6502_PYTHON_FOUND TRUE)
        set(QE6502_PYTHON_USING_STABLE_ABI_TARGET TRUE)
    endif()
endif()

if(NOT QE6502_PYTHON_FOUND)
    if(QE6502_REQUIRE_PYTHON)
        find_package(Python3 REQUIRED COMPONENTS Development.Module)
    else()
        find_package(Python3 COMPONENTS Development.Module)
    endif()

    if(Python3_Development.Module_FOUND)
        set(QE6502_PYTHON_MODULE_TARGET Python3::Module)
        set(QE6502_PYTHON_FOUND TRUE)
        set(QE6502_PYTHON_USING_STABLE_ABI_TARGET FALSE)
    endif()
endif()

if(NOT QE6502_PYTHON_FOUND)
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

target_compile_definitions(_qe6502
    PRIVATE
        Py_LIMITED_API=${QE6502_PYTHON_LIMITED_API_HEX}
)

target_link_libraries(_qe6502
    PRIVATE
        ${QE6502_PYTHON_MODULE_TARGET}
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
elseif(QE6502_PYTHON_USING_STABLE_ABI_TARGET AND DEFINED Python3_SOSABI AND NOT Python3_SOSABI STREQUAL "")
    if(Python3_SOSABI MATCHES "^\\.")
        set(QE6502_PYTHON_EXTENSION_SUFFIX "${Python3_SOSABI}")
    else()
        set(QE6502_PYTHON_EXTENSION_SUFFIX ".${Python3_SOSABI}${CMAKE_SHARED_MODULE_SUFFIX}")
    endif()
    set_target_properties(_qe6502 PROPERTIES SUFFIX "${QE6502_PYTHON_EXTENSION_SUFFIX}")
elseif(QE6502_PYTHON_USING_STABLE_ABI_TARGET)
    set_target_properties(_qe6502 PROPERTIES SUFFIX ".abi3${CMAKE_SHARED_MODULE_SUFFIX}")
elseif(DEFINED Python3_EXTENSION_SUFFIX AND NOT Python3_EXTENSION_SUFFIX STREQUAL "")
    set_target_properties(_qe6502 PROPERTIES SUFFIX "${Python3_EXTENSION_SUFFIX}")
endif()

message(STATUS "qe6502 Python binding uses CPython Limited API ${QE6502_PYTHON_STABLE_ABI_MINIMUM}+; stable ABI target: ${QE6502_PYTHON_USING_STABLE_ABI_TARGET}")

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
