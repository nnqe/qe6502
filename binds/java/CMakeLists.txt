include(Qe6502FindJava25)

if(NOT QE6502_BUILD_JAVA)
    message(STATUS "QE6502_BUILD_JAVA is OFF; skipping qe6502 Java binding")
    return()
endif()

if(QE6502_REQUIRE_JAVA)
    qe6502_find_java25(REQUIRED)
else()
    qe6502_find_java25()
endif()

if(NOT Java_FOUND)
    message(STATUS "Java 25+ development tools not found; skipping qe6502 Java binding")
    return()
endif()

if(NOT TARGET qe6502_shared)
    message(STATUS "qe6502_shared target is not available; skipping qe6502 Java binding")
    return()
endif()

set(QE6502_JAVA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/qe6502-java/src/main/java)
set(QE6502_JAVA_CLASSES_DIR ${CMAKE_CURRENT_BINARY_DIR}/classes)
set(QE6502_JAVA_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dist)
set(QE6502_JAVA_JAR ${QE6502_JAVA_OUTPUT_DIR}/qe6502-java.jar)

file(GLOB_RECURSE QE6502_JAVA_SOURCES CONFIGURE_DEPENDS
    ${QE6502_JAVA_SOURCE_DIR}/*.java
)

if(WIN32)
    set(QE6502_JAVA_NATIVE_LIBRARY_NAME "$<TARGET_FILE_NAME:qe6502_shared>")
else()
    set(QE6502_JAVA_NATIVE_LIBRARY_NAME "$<TARGET_LINKER_FILE_NAME:qe6502_shared>")
endif()

add_custom_command(
    OUTPUT ${QE6502_JAVA_JAR}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${QE6502_JAVA_CLASSES_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_JAVA_CLASSES_DIR}
    COMMAND ${Java_JAVAC_EXECUTABLE}
        --release 25
        -d ${QE6502_JAVA_CLASSES_DIR}
        ${QE6502_JAVA_SOURCES}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_JAVA_OUTPUT_DIR}
    COMMAND ${Java_JAR_EXECUTABLE}
        --create
        --file ${QE6502_JAVA_JAR}
        -C ${QE6502_JAVA_CLASSES_DIR} .
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:qe6502_shared>
        ${QE6502_JAVA_OUTPUT_DIR}/${QE6502_JAVA_NATIVE_LIBRARY_NAME}
    DEPENDS
        qe6502_shared
        ${QE6502_JAVA_SOURCES}
    COMMENT "Building qe6502 Java binding"
    VERBATIM
)

add_custom_target(qe6502_java ALL
    DEPENDS ${QE6502_JAVA_JAR}
)

set(QE6502_JAVA_JAR ${QE6502_JAVA_JAR} CACHE INTERNAL "qe6502 Java binding jar")
set(QE6502_JAVA_OUTPUT_DIR ${QE6502_JAVA_OUTPUT_DIR} CACHE INTERNAL "qe6502 Java binding output directory")
set(QE6502_JAVA_NATIVE_LIBRARY_NAME ${QE6502_JAVA_NATIVE_LIBRARY_NAME} CACHE INTERNAL "qe6502 Java native library filename")
