cmake_minimum_required(VERSION 3.15)

function(qe6502_run_step step_name)
    execute_process(
        COMMAND ${ARGN}
        RESULT_VARIABLE QE6502_STEP_RESULT
        OUTPUT_VARIABLE QE6502_STEP_OUTPUT
        ERROR_VARIABLE QE6502_STEP_ERROR
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
    )

    if(NOT QE6502_STEP_RESULT EQUAL 0)
        message(FATAL_ERROR "${step_name} failed with exit code ${QE6502_STEP_RESULT}")
    endif()
endfunction()

foreach(required_var IN ITEMS
    QE6502_SOURCE_DIR
    QE6502_WORK_DIR
    QE6502_CONSUMER_SOURCE_DIR
    QE6502_GENERATOR
)
    if(NOT DEFINED ${required_var} OR "${${required_var}}" STREQUAL "")
        message(FATAL_ERROR "${required_var} is required")
    endif()
endforeach()

if(NOT DEFINED QE6502_TEST_CONFIG OR "${QE6502_TEST_CONFIG}" STREQUAL "")
    set(QE6502_TEST_CONFIG Debug)
endif()

set(QE6502_PACKAGE_BUILD_DIR "${QE6502_WORK_DIR}/qe6502-build")
set(QE6502_INSTALL_PREFIX "${QE6502_WORK_DIR}/qe6502-install")
set(QE6502_CONSUMER_BUILD_DIR "${QE6502_WORK_DIR}/consumer-build")

file(REMOVE_RECURSE
    "${QE6502_PACKAGE_BUILD_DIR}"
    "${QE6502_INSTALL_PREFIX}"
    "${QE6502_CONSUMER_BUILD_DIR}"
)
file(MAKE_DIRECTORY "${QE6502_WORK_DIR}")

set(QE6502_GENERATOR_ARGS -G "${QE6502_GENERATOR}")
if(DEFINED QE6502_GENERATOR_PLATFORM AND NOT "${QE6502_GENERATOR_PLATFORM}" STREQUAL "")
    list(APPEND QE6502_GENERATOR_ARGS -A "${QE6502_GENERATOR_PLATFORM}")
endif()
if(DEFINED QE6502_GENERATOR_TOOLSET AND NOT "${QE6502_GENERATOR_TOOLSET}" STREQUAL "")
    list(APPEND QE6502_GENERATOR_ARGS -T "${QE6502_GENERATOR_TOOLSET}")
endif()

message(STATUS "Configuring qe6502 dynamic-only package build")
qe6502_run_step("configure qe6502 dynamic package"
    "${CMAKE_COMMAND}"
    -S "${QE6502_SOURCE_DIR}"
    -B "${QE6502_PACKAGE_BUILD_DIR}"
    ${QE6502_GENERATOR_ARGS}
    -DCMAKE_BUILD_TYPE=${QE6502_TEST_CONFIG}
    -DCMAKE_INSTALL_PREFIX=${QE6502_INSTALL_PREFIX}
    -DBUILD_TESTING=OFF
    -DQE6502_BUILD_STATIC=OFF
    -DQE6502_BUILD_SHARED=ON
    -DQE6502_BUILD_CPP=ON
    -DQE6502_BUILD_TOOLS=OFF
    -DQE6502_BUILD_TESTS=OFF
    -DQE6502_BUILD_CSHARP=OFF
    -DQE6502_BUILD_RUST=OFF
    -DQE6502_BUILD_JAVA=OFF
    -DQE6502_BUILD_PYTHON=OFF
    -DQE6502_INSTALL=ON
)

message(STATUS "Building qe6502 dynamic-only package build")
qe6502_run_step("build qe6502 dynamic package"
    "${CMAKE_COMMAND}" --build "${QE6502_PACKAGE_BUILD_DIR}" --config "${QE6502_TEST_CONFIG}"
)

message(STATUS "Installing qe6502 dynamic-only package build")
qe6502_run_step("install qe6502 dynamic package"
    "${CMAKE_COMMAND}" --install "${QE6502_PACKAGE_BUILD_DIR}" --config "${QE6502_TEST_CONFIG}"
)

message(STATUS "Configuring installed-package C++ consumer")
qe6502_run_step("configure installed-package C++ consumer"
    "${CMAKE_COMMAND}"
    -S "${QE6502_CONSUMER_SOURCE_DIR}"
    -B "${QE6502_CONSUMER_BUILD_DIR}"
    ${QE6502_GENERATOR_ARGS}
    -DCMAKE_BUILD_TYPE=${QE6502_TEST_CONFIG}
    -DCMAKE_PREFIX_PATH=${QE6502_INSTALL_PREFIX}
    -DQE6502_INSTALL_PREFIX=${QE6502_INSTALL_PREFIX}
)

message(STATUS "Building installed-package C++ consumer")
qe6502_run_step("build installed-package C++ consumer"
    "${CMAKE_COMMAND}" --build "${QE6502_CONSUMER_BUILD_DIR}" --config "${QE6502_TEST_CONFIG}"
)

set(QE6502_CONSUMER_EXE "${QE6502_CONSUMER_BUILD_DIR}/qe6502_install_consume_cpp")
if(WIN32)
    set(QE6502_CONSUMER_EXE "${QE6502_CONSUMER_BUILD_DIR}/${QE6502_TEST_CONFIG}/qe6502_install_consume_cpp.exe")
elseif(EXISTS "${QE6502_CONSUMER_BUILD_DIR}/${QE6502_TEST_CONFIG}/qe6502_install_consume_cpp")
    set(QE6502_CONSUMER_EXE "${QE6502_CONSUMER_BUILD_DIR}/${QE6502_TEST_CONFIG}/qe6502_install_consume_cpp")
endif()

if(NOT EXISTS "${QE6502_CONSUMER_EXE}")
    message(FATAL_ERROR "Consumer executable was not produced: ${QE6502_CONSUMER_EXE}")
endif()

set(QE6502_RUNTIME_PATH "${QE6502_INSTALL_PREFIX}/bin")
if(WIN32)
    set(QE6502_PATH_SEP ";")
else()
    set(QE6502_PATH_SEP ":")
endif()

message(STATUS "Running installed-package C++ consumer")
qe6502_run_step("run installed-package C++ consumer"
    "${CMAKE_COMMAND}" -E env
        "PATH=${QE6502_RUNTIME_PATH}${QE6502_PATH_SEP}$ENV{PATH}"
        "LD_LIBRARY_PATH=${QE6502_INSTALL_PREFIX}/lib${QE6502_PATH_SEP}$ENV{LD_LIBRARY_PATH}"
        "DYLD_LIBRARY_PATH=${QE6502_INSTALL_PREFIX}/lib${QE6502_PATH_SEP}$ENV{DYLD_LIBRARY_PATH}"
        "${QE6502_CONSUMER_EXE}"
)
