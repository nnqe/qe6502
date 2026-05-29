# /CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(qe6502 VERSION 0.1.0 LANGUAGES C)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(QE6502_BUILD_WASM "Enable WebAssembly target mode" OFF)
option(QE6502_BUILD_HARNESS "Build native harness tools" ON)
option(QE6502_BUILD_BIND_JS "Enable JavaScript bind build" OFF)
option(QE6502_BUILD_BIND_CPP "Enable CPP bind build" OFF)
option(QE6502_BUILD_BIND_PYTHON "Enable Python bind build" OFF)

message(STATUS "  QE6502_BUILD_WASM        = ${QE6502_BUILD_WASM}")
message(STATUS "  QE6502_BUILD_HARNESS     = ${QE6502_BUILD_HARNESS}")
message(STATUS "  QE6502_BUILD_BIND_JS     = ${QE6502_BUILD_BIND_JS}")
message(STATUS "  QE6502_BUILD_BIND_CPP    = ${QE6502_BUILD_BIND_CPP}")
message(STATUS "  QE6502_BUILD_BIND_PYTHON = ${QE6502_BUILD_BIND_PYTHON}")

add_subdirectory(cpu)

# Bindings stay disabled for now. We will re-enable them explicitly later.
# add_subdirectory(binds)

if(QE6502_BUILD_HARNESS AND NOT QE6502_BUILD_WASM)
    enable_language(CXX)
    add_subdirectory(harness)
endif()

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qe6502Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/qe6502Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qe6502
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/qe6502ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT qe6502Targets
    NAMESPACE qe6502::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qe6502
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/qe6502Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/qe6502ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qe6502
)
