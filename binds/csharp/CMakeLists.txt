find_program(DOTNET_EXECUTABLE dotnet)

if(NOT DOTNET_EXECUTABLE)
    message(STATUS "dotnet not found; skipping qe6502 C# binding")
    return()
endif()

if(NOT TARGET qe6502_shared)
    message(STATUS "qe6502_shared target is not available; skipping qe6502 C# binding")
    return()
endif()

set(QE6502_CSHARP_CONFIGURATION "$<IF:$<CONFIG:Debug>,Debug,Release>")
set(QE6502_CSHARP_PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/Qe6502/Qe6502.csproj)
set(QE6502_CSHARP_OUTPUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/Qe6502/bin/${QE6502_CSHARP_CONFIGURATION}/netstandard2.0
)

if(WIN32)
    set(QE6502_CSHARP_NATIVE_LIBRARY_NAME "qe6502.dll")
else()
    set(QE6502_CSHARP_NATIVE_LIBRARY_NAME "$<TARGET_LINKER_FILE_NAME:qe6502_shared>")
endif()

add_custom_target(qe6502_csharp ALL
    COMMAND ${DOTNET_EXECUTABLE} build ${QE6502_CSHARP_PROJECT}
        --configuration ${QE6502_CSHARP_CONFIGURATION}
        -p:Qe6502CMakeArtifactsRoot=${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_CSHARP_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:qe6502_shared>
        ${QE6502_CSHARP_OUTPUT_DIR}/${QE6502_CSHARP_NATIVE_LIBRARY_NAME}
    DEPENDS qe6502_shared
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building qe6502 C# binding"
    VERBATIM
)
