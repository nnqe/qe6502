set(QE6502_LLVM_ROOT "" CACHE PATH "Optional path to LLVM installation root")

set(_QE6502_LLVM_HINTS)

if(QE6502_LLVM_ROOT)
    list(APPEND _QE6502_LLVM_HINTS "${QE6502_LLVM_ROOT}/bin")
endif()

if(DEFINED ENV{LLVM_ROOT})
    list(APPEND _QE6502_LLVM_HINTS "$ENV{LLVM_ROOT}/bin")
endif()

if(DEFINED ENV{LLVM_DIR})
    list(APPEND _QE6502_LLVM_HINTS "$ENV{LLVM_DIR}/bin")
endif()

find_program(_QE6502_BREW_EXECUTABLE brew)

if(_QE6502_BREW_EXECUTABLE)
    execute_process(
        COMMAND "${_QE6502_BREW_EXECUTABLE}" --prefix llvm
        OUTPUT_VARIABLE _QE6502_BREW_LLVM_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _QE6502_BREW_LLVM_RESULT
    )

    if(_QE6502_BREW_LLVM_RESULT EQUAL 0 AND _QE6502_BREW_LLVM_PREFIX)
        list(APPEND _QE6502_LLVM_HINTS "${_QE6502_BREW_LLVM_PREFIX}/bin")
    endif()
endif()

find_program(QE6502_CLANG
    NAMES
        clang
        clang-22 clang-21 clang-20 clang-19 clang-18 clang-17 clang-16 clang-15 clang-14
    HINTS
        ${_QE6502_LLVM_HINTS}
    REQUIRED
)

find_program(QE6502_LLVM_AR
    NAMES
        llvm-ar
        llvm-ar-22 llvm-ar-21 llvm-ar-20 llvm-ar-19 llvm-ar-18 llvm-ar-17 llvm-ar-16 llvm-ar-15 llvm-ar-14
    HINTS
        ${_QE6502_LLVM_HINTS}
    REQUIRED
)

find_program(QE6502_LLVM_RANLIB
    NAMES
        llvm-ranlib
        llvm-ranlib-22 llvm-ranlib-21 llvm-ranlib-20 llvm-ranlib-19 llvm-ranlib-18 llvm-ranlib-17 llvm-ranlib-16 llvm-ranlib-15 llvm-ranlib-14
    HINTS
        ${_QE6502_LLVM_HINTS}
    REQUIRED
)

message(STATUS "QE6502 LLVM clang: ${QE6502_CLANG}")
message(STATUS "QE6502 LLVM ar: ${QE6502_LLVM_AR}")
message(STATUS "QE6502 LLVM ranlib: ${QE6502_LLVM_RANLIB}")

unset(_QE6502_LLVM_HINTS)
unset(_QE6502_BREW_EXECUTABLE)
unset(_QE6502_BREW_LLVM_PREFIX)
unset(_QE6502_BREW_LLVM_RESULT)
