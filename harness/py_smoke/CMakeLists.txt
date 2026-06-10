find_package(Python3 COMPONENTS Interpreter)

if(NOT QE6502_BUILD_PYTHON)
    message(STATUS "QE6502_BUILD_PYTHON is OFF; skipping qe6502 Python smoke harness")
    return()
endif()

if(NOT Python3_FOUND)
    message(STATUS "Python3 interpreter not found; skipping qe6502 Python smoke harness")
    return()
endif()

if(NOT TARGET qe6502_python)
    message(STATUS "qe6502_python target is not available; skipping qe6502 Python smoke harness")
    return()
endif()

set(QE6502_PYTHONPATH ${CMAKE_BINARY_DIR}/binds/python)

add_custom_target(qe6502_py_smoke_run
    COMMAND ${CMAKE_COMMAND} -E env
        PYTHONPATH=${QE6502_PYTHONPATH}
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/py_smoke.py
    DEPENDS qe6502_python
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running qe6502 Python smoke harness"
    VERBATIM
)

add_test(NAME qe6502_py_smoke
    COMMAND ${CMAKE_COMMAND} -E env
        PYTHONPATH=${QE6502_PYTHONPATH}
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/py_smoke.py
)

set_tests_properties(qe6502_py_smoke PROPERTIES
    LABELS "python;smoke;functional"
    TIMEOUT 30
)
