find_package(Python3 COMPONENTS Interpreter)

if(NOT QE6502_BUILD_PYTHON)
    message(STATUS "QE6502_BUILD_PYTHON is OFF; skipping qe6502 Python Klaus2m5 harness")
    return()
endif()

if(NOT Python3_FOUND)
    message(STATUS "Python3 interpreter not found; skipping qe6502 Python Klaus2m5 harness")
    return()
endif()

if(NOT TARGET qe6502_python)
    message(STATUS "qe6502_python target is not available; skipping qe6502 Python Klaus2m5 harness")
    return()
endif()

set(QE6502_PYTHONPATH ${CMAKE_BINARY_DIR}/binds/python)
set(QE6502_PY_KLAUS_ROM_DIR ${CMAKE_SOURCE_DIR}/harness/klaus2m5)
set(QE6502_PY_KLAUS_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/py_klaus2m5.py)

add_custom_target(qe6502_py_klaus2m5_run
    COMMAND ${CMAKE_COMMAND} -E env
        PYTHONPATH=${QE6502_PYTHONPATH}
        ${Python3_EXECUTABLE} ${QE6502_PY_KLAUS_SCRIPT} ${QE6502_PY_KLAUS_ROM_DIR}
    DEPENDS qe6502_python
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running qe6502 Python Klaus2m5 harness"
    VERBATIM
)

set(QE6502_PY_KLAUS2M5_TESTS
    nmos standard
    wdc standard
    wdc extended
    rw standard
    rw extended
    st standard
)

list(LENGTH QE6502_PY_KLAUS2M5_TESTS QE6502_PY_KLAUS2M5_TEST_COUNT)
math(EXPR QE6502_PY_KLAUS2M5_LAST_INDEX "${QE6502_PY_KLAUS2M5_TEST_COUNT} - 1")

foreach(index RANGE 0 ${QE6502_PY_KLAUS2M5_LAST_INDEX} 2)
    list(GET QE6502_PY_KLAUS2M5_TESTS ${index} model)
    math(EXPR test_index "${index} + 1")
    list(GET QE6502_PY_KLAUS2M5_TESTS ${test_index} test_name)

    add_test(
        NAME qe6502.py.klaus2m5.${model}.${test_name}
        COMMAND ${CMAKE_COMMAND} -E env
            PYTHONPATH=${QE6502_PYTHONPATH}
            ${Python3_EXECUTABLE} ${QE6502_PY_KLAUS_SCRIPT} ${QE6502_PY_KLAUS_ROM_DIR} ${model} ${test_name}
    )

    set_tests_properties(qe6502.py.klaus2m5.${model}.${test_name} PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        LABELS "python;v2;klaus;functional;performance"
        TIMEOUT 180
    )
endforeach()
