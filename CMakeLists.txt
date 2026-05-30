# /CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(qe6502 VERSION 0.1.0 LANGUAGES C)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(QE6502_BUILD_WASM "Enable WebAssembly target mode" OFF)

message(STATUS "  QE6502_BUILD_WASM        = ${QE6502_BUILD_WASM}")

add_subdirectory(cpu)
add_subdirectory(binds)
add_subdirectory(harness)

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
