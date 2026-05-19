/*
 * QE public API platform / ABI support.
 *
 * This file intentionally detects the TARGET platform/architecture, not the
 * build host. For example, when compiling on macOS with Emscripten for WASM,
 * QE_TARGET_PLATFORM_WASM_ and QE_TARGET_ARCH_WASM32_ should be set, not
 * QE_TARGET_PLATFORM_MACOS_.
 */

#ifndef QE_API_MACROS_H
#define QE_API_MACROS_H

/* ------------------------------------------------------------------------- */
/* Compiler / toolchain detection                                             */
/* ------------------------------------------------------------------------- */

#if defined(__clang__)
#   define QE_COMPILER_CLANG_ 1
#else
#   define QE_COMPILER_CLANG_ 0
#endif

#if defined(__INTEL_LLVM_COMPILER)
#   define QE_COMPILER_INTEL_LLVM_ 1
#else
#   define QE_COMPILER_INTEL_LLVM_ 0
#endif

#if defined(__INTEL_COMPILER) || defined(__ICC) || defined(__ICL)
#   define QE_COMPILER_INTEL_CLASSIC_ 1
#else
#   define QE_COMPILER_INTEL_CLASSIC_ 0
#endif

#if QE_COMPILER_CLANG_ && defined(_MSC_VER)
#   define QE_COMPILER_CLANG_CL_ 1
#else
#   define QE_COMPILER_CLANG_CL_ 0
#endif

#if defined(_MSC_VER) && \
    !QE_COMPILER_CLANG_ && \
    !QE_COMPILER_INTEL_LLVM_ && \
    !QE_COMPILER_INTEL_CLASSIC_
#   define QE_COMPILER_MSVC_ 1
#else
#   define QE_COMPILER_MSVC_ 0
#endif

#if defined(_MSC_VER)
#   define QE_COMPILER_MSVC_COMPAT_ 1
#else
#   define QE_COMPILER_MSVC_COMPAT_ 0
#endif

#if defined(__GNUC__) && \
    !QE_COMPILER_CLANG_ && \
    !QE_COMPILER_INTEL_LLVM_ && \
    !QE_COMPILER_INTEL_CLASSIC_
#   define QE_COMPILER_GCC_ 1
#else
#   define QE_COMPILER_GCC_ 0
#endif

#if defined(__GNUC__) || QE_COMPILER_CLANG_
#   define QE_COMPILER_GNUC_COMPAT_ 1
#else
#   define QE_COMPILER_GNUC_COMPAT_ 0
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
#   define QE_COMPILER_MINGW_ 1
#else
#   define QE_COMPILER_MINGW_ 0
#endif

#if defined(__EMSCRIPTEN__)
#   define QE_COMPILER_EMSCRIPTEN_ 1
#else
#   define QE_COMPILER_EMSCRIPTEN_ 0
#endif

#if defined(__TINYC__)
#   define QE_COMPILER_TINYC_ 1
#else
#   define QE_COMPILER_TINYC_ 0
#endif

#if !QE_COMPILER_CLANG_ && \
    !QE_COMPILER_MSVC_ && \
    !QE_COMPILER_GCC_ && \
    !QE_COMPILER_INTEL_LLVM_ && \
    !QE_COMPILER_INTEL_CLASSIC_ && \
    !QE_COMPILER_TINYC_
#   define QE_COMPILER_UNKNOWN_ 1
#else
#   define QE_COMPILER_UNKNOWN_ 0
#endif

/* ------------------------------------------------------------------------- */
/* Target platform detection                                                  */
/* ------------------------------------------------------------------------- */

#if defined(__EMSCRIPTEN__) || defined(__wasi__) || defined(__wasm__) || defined(__wasm32__) || defined(__wasm64__)
#   define QE_TARGET_PLATFORM_WASM_ 1
#else
#   define QE_TARGET_PLATFORM_WASM_ 0
#endif

#if defined(__wasi__)
#   define QE_TARGET_PLATFORM_WASI_ 1
#else
#   define QE_TARGET_PLATFORM_WASI_ 0
#endif

#if defined(__CYGWIN__)
#   define QE_TARGET_PLATFORM_CYGWIN_ 1
#else
#   define QE_TARGET_PLATFORM_CYGWIN_ 0
#endif

#if defined(_WIN32) && !QE_TARGET_PLATFORM_CYGWIN_ && !QE_TARGET_PLATFORM_WASM_
#   define QE_TARGET_PLATFORM_WINDOWS_ 1
#else
#   define QE_TARGET_PLATFORM_WINDOWS_ 0
#endif

#if defined(__ANDROID__) && !QE_TARGET_PLATFORM_WASM_
#   define QE_TARGET_PLATFORM_ANDROID_ 1
#else
#   define QE_TARGET_PLATFORM_ANDROID_ 0
#endif

#if (defined(__linux__) || defined(__linux)) && !QE_TARGET_PLATFORM_ANDROID_ && !QE_TARGET_PLATFORM_WASM_
#   define QE_TARGET_PLATFORM_LINUX_ 1
#else
#   define QE_TARGET_PLATFORM_LINUX_ 0
#endif

#if defined(__APPLE__) && defined(__MACH__) && !QE_TARGET_PLATFORM_WASM_
#   include <TargetConditionals.h>
#   define QE_TARGET_PLATFORM_APPLE_ 1
#else
#   define QE_TARGET_PLATFORM_APPLE_ 0
#endif

#if QE_TARGET_PLATFORM_APPLE_ && defined(TARGET_OS_TV) && TARGET_OS_TV
#   define QE_TARGET_PLATFORM_TVOS_ 1
#else
#   define QE_TARGET_PLATFORM_TVOS_ 0
#endif

#if QE_TARGET_PLATFORM_APPLE_ && defined(TARGET_OS_WATCH) && TARGET_OS_WATCH
#   define QE_TARGET_PLATFORM_WATCHOS_ 1
#else
#   define QE_TARGET_PLATFORM_WATCHOS_ 0
#endif

#if QE_TARGET_PLATFORM_APPLE_ && defined(TARGET_OS_VISION) && TARGET_OS_VISION
#   define QE_TARGET_PLATFORM_VISIONOS_ 1
#else
#   define QE_TARGET_PLATFORM_VISIONOS_ 0
#endif

#if QE_TARGET_PLATFORM_APPLE_ && \
    ((defined(TARGET_OS_IOS) && TARGET_OS_IOS) || \
     ((defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) && \
      !QE_TARGET_PLATFORM_TVOS_ && \
      !QE_TARGET_PLATFORM_WATCHOS_ && \
      !QE_TARGET_PLATFORM_VISIONOS_))
#   define QE_TARGET_PLATFORM_IOS_ 1
#else
#   define QE_TARGET_PLATFORM_IOS_ 0
#endif

#if QE_TARGET_PLATFORM_APPLE_ && \
    ((defined(TARGET_OS_OSX) && TARGET_OS_OSX) || \
     ((defined(TARGET_OS_MAC) && TARGET_OS_MAC) && \
      !QE_TARGET_PLATFORM_IOS_ && \
      !QE_TARGET_PLATFORM_TVOS_ && \
      !QE_TARGET_PLATFORM_WATCHOS_ && \
      !QE_TARGET_PLATFORM_VISIONOS_))
#   define QE_TARGET_PLATFORM_MACOS_ 1
#else
#   define QE_TARGET_PLATFORM_MACOS_ 0
#endif

#if defined(__FreeBSD__)
#   define QE_TARGET_PLATFORM_FREEBSD_ 1
#else
#   define QE_TARGET_PLATFORM_FREEBSD_ 0
#endif

#if defined(__OpenBSD__)
#   define QE_TARGET_PLATFORM_OPENBSD_ 1
#else
#   define QE_TARGET_PLATFORM_OPENBSD_ 0
#endif

#if defined(__NetBSD__)
#   define QE_TARGET_PLATFORM_NETBSD_ 1
#else
#   define QE_TARGET_PLATFORM_NETBSD_ 0
#endif

#if defined(__DragonFly__)
#   define QE_TARGET_PLATFORM_DRAGONFLYBSD_ 1
#else
#   define QE_TARGET_PLATFORM_DRAGONFLYBSD_ 0
#endif

#if QE_TARGET_PLATFORM_FREEBSD_ || \
    QE_TARGET_PLATFORM_OPENBSD_ || \
    QE_TARGET_PLATFORM_NETBSD_ || \
    QE_TARGET_PLATFORM_DRAGONFLYBSD_
#   define QE_TARGET_PLATFORM_BSD_ 1
#else
#   define QE_TARGET_PLATFORM_BSD_ 0
#endif

#if defined(__sun) && defined(__SVR4)
#   define QE_TARGET_PLATFORM_SOLARIS_ 1
#else
#   define QE_TARGET_PLATFORM_SOLARIS_ 0
#endif

#if defined(__HAIKU__)
#   define QE_TARGET_PLATFORM_HAIKU_ 1
#else
#   define QE_TARGET_PLATFORM_HAIKU_ 0
#endif

#if QE_TARGET_PLATFORM_LINUX_ || \
    QE_TARGET_PLATFORM_ANDROID_ || \
    QE_TARGET_PLATFORM_APPLE_ || \
    QE_TARGET_PLATFORM_BSD_ || \
    QE_TARGET_PLATFORM_CYGWIN_ || \
    QE_TARGET_PLATFORM_SOLARIS_ || \
    QE_TARGET_PLATFORM_HAIKU_
#   define QE_TARGET_PLATFORM_POSIX_ 1
#else
#   define QE_TARGET_PLATFORM_POSIX_ 0
#endif

#if !QE_TARGET_PLATFORM_WINDOWS_ && \
    !QE_TARGET_PLATFORM_CYGWIN_ && \
    !QE_TARGET_PLATFORM_LINUX_ && \
    !QE_TARGET_PLATFORM_ANDROID_ && \
    !QE_TARGET_PLATFORM_APPLE_ && \
    !QE_TARGET_PLATFORM_WASM_ && \
    !QE_TARGET_PLATFORM_BSD_ && \
    !QE_TARGET_PLATFORM_SOLARIS_ && \
    !QE_TARGET_PLATFORM_HAIKU_
#   define QE_TARGET_PLATFORM_UNKNOWN_ 1
#else
#   define QE_TARGET_PLATFORM_UNKNOWN_ 0
#endif

/* ------------------------------------------------------------------------- */
/* Target CPU / ISA detection                                                 */
/* ------------------------------------------------------------------------- */

#if defined(__wasm32__)
#   define QE_TARGET_ARCH_WASM32_ 1
#else
#   define QE_TARGET_ARCH_WASM32_ 0
#endif

#if defined(__wasm64__)
#   define QE_TARGET_ARCH_WASM64_ 1
#else
#   define QE_TARGET_ARCH_WASM64_ 0
#endif

#if defined(_M_IX86) || defined(__i386__) || defined(__i386)
#   define QE_TARGET_ARCH_X86_ 1
#else
#   define QE_TARGET_ARCH_X86_ 0
#endif

#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64)
#   define QE_TARGET_ARCH_X64_ 1
#else
#   define QE_TARGET_ARCH_X64_ 0
#endif

#if defined(_M_ARM64EC)
#   define QE_TARGET_ARCH_ARM64EC_ 1
#else
#   define QE_TARGET_ARCH_ARM64EC_ 0
#endif

#if defined(_M_ARM64) || defined(__aarch64__) || defined(__arm64__) || QE_TARGET_ARCH_ARM64EC_
#   define QE_TARGET_ARCH_ARM64_ 1
#else
#   define QE_TARGET_ARCH_ARM64_ 0
#endif

#if defined(_M_ARM) || defined(__arm__) || defined(__thumb__)
#   define QE_TARGET_ARCH_ARM_ 1
#else
#   define QE_TARGET_ARCH_ARM_ 0
#endif

#if defined(__riscv) && defined(__riscv_xlen) && (__riscv_xlen == 32)
#   define QE_TARGET_ARCH_RISCV32_ 1
#else
#   define QE_TARGET_ARCH_RISCV32_ 0
#endif

#if defined(__riscv) && defined(__riscv_xlen) && (__riscv_xlen == 64)
#   define QE_TARGET_ARCH_RISCV64_ 1
#else
#   define QE_TARGET_ARCH_RISCV64_ 0
#endif

#if defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) || defined(_ARCH_PPC64)
#   define QE_TARGET_ARCH_PPC64_ 1
#else
#   define QE_TARGET_ARCH_PPC64_ 0
#endif

#if (defined(__powerpc__) || defined(__ppc__) || defined(__PPC__) || defined(_M_PPC) || defined(_ARCH_PPC)) && !QE_TARGET_ARCH_PPC64_
#   define QE_TARGET_ARCH_PPC_ 1
#else
#   define QE_TARGET_ARCH_PPC_ 0
#endif

#if defined(__mips64) || defined(__mips64__) || defined(_MIPS_ARCH_MIPS64)
#   define QE_TARGET_ARCH_MIPS64_ 1
#else
#   define QE_TARGET_ARCH_MIPS64_ 0
#endif

#if (defined(__mips__) || defined(__mips) || defined(_MIPS_ARCH_MIPS32)) && !QE_TARGET_ARCH_MIPS64_
#   define QE_TARGET_ARCH_MIPS_ 1
#else
#   define QE_TARGET_ARCH_MIPS_ 0
#endif

/*
 * QE_TARGET_ARCH_32BIT_ and QE_TARGET_ARCH_64BIT_ describe the target ABI /
 * pointer width when it can be detected. They are not merely ISA-family flags.
 * For example, an x86_64 ILP32/x32-style target may set QE_TARGET_ARCH_X64_
 * while still being a 32-bit ABI.
 */
#if defined(_WIN64) || \
    defined(__LP64__) || \
    defined(_LP64) || \
    (defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8))
#   define QE_TARGET_ARCH_64BIT_ 1
#else
#   define QE_TARGET_ARCH_64BIT_ 0
#endif

#if (!QE_TARGET_ARCH_64BIT_ && defined(_WIN32)) || \
    defined(__ILP32__) || \
    (defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 4))
#   define QE_TARGET_ARCH_32BIT_ 1
#else
#   define QE_TARGET_ARCH_32BIT_ 0
#endif

#if !QE_TARGET_ARCH_32BIT_ && !QE_TARGET_ARCH_64BIT_
#   if  QE_TARGET_ARCH_X64_ || \
        QE_TARGET_ARCH_ARM64_ || \
        QE_TARGET_ARCH_WASM64_ || \
        QE_TARGET_ARCH_RISCV64_ || \
        QE_TARGET_ARCH_PPC64_ || \
        QE_TARGET_ARCH_MIPS64_

#       undef  QE_TARGET_ARCH_64BIT_
#       define QE_TARGET_ARCH_64BIT_ 1

#   elif QE_TARGET_ARCH_X86_ || \
         QE_TARGET_ARCH_ARM_ || \
         QE_TARGET_ARCH_WASM32_ || \
         QE_TARGET_ARCH_RISCV32_ || \
         QE_TARGET_ARCH_PPC_ || \
         QE_TARGET_ARCH_MIPS_

#       undef  QE_TARGET_ARCH_32BIT_
#       define QE_TARGET_ARCH_32BIT_ 1

#   endif
#endif

#if !QE_TARGET_ARCH_X86_ && \
    !QE_TARGET_ARCH_X64_ && \
    !QE_TARGET_ARCH_ARM_ && \
    !QE_TARGET_ARCH_ARM64_ && \
    !QE_TARGET_ARCH_WASM32_ && \
    !QE_TARGET_ARCH_WASM64_ && \
    !QE_TARGET_ARCH_RISCV32_ && \
    !QE_TARGET_ARCH_RISCV64_ && \
    !QE_TARGET_ARCH_PPC_ && \
    !QE_TARGET_ARCH_PPC64_ && \
    !QE_TARGET_ARCH_MIPS_ && \
    !QE_TARGET_ARCH_MIPS64_
#   define QE_TARGET_ARCH_UNKNOWN_ 1
#else
#   define QE_TARGET_ARCH_UNKNOWN_ 0
#endif

/* ------------------------------------------------------------------------- */
/* Compiler / language feature helpers                                        */
/* ------------------------------------------------------------------------- */

#if (QE_TARGET_PLATFORM_WINDOWS_ || QE_TARGET_PLATFORM_CYGWIN_) && \
    (QE_COMPILER_MSVC_COMPAT_ || QE_COMPILER_MINGW_ || QE_COMPILER_GNUC_COMPAT_)
#   define QE_HAS_DECLSPEC_DLLEXPORT_ 1
#   define QE_HAS_DECLSPEC_DLLIMPORT_ 1
#else
#   define QE_HAS_DECLSPEC_DLLEXPORT_ 0
#   define QE_HAS_DECLSPEC_DLLIMPORT_ 0
#endif

#if QE_COMPILER_MSVC_COMPAT_
#   define QE_HAS_DECLSPEC_ALIGN_ 1
#   define QE_HAS_DECLSPEC_NOINLINE_ 1
#   define QE_HAS_MSVC_ASSUME_ 1
#else
#   define QE_HAS_DECLSPEC_ALIGN_ 0
#   define QE_HAS_DECLSPEC_NOINLINE_ 0
#   define QE_HAS_MSVC_ASSUME_ 0
#endif

#if defined(__has_attribute)
#   if __has_attribute(visibility)
#       define QE_HAS_ATTRIBUTE_VISIBILITY_ 1
#   else
#       define QE_HAS_ATTRIBUTE_VISIBILITY_ 0
#   endif
#else
#   if QE_COMPILER_GNUC_COMPAT_
#       define QE_HAS_ATTRIBUTE_VISIBILITY_ 1
#   else
#       define QE_HAS_ATTRIBUTE_VISIBILITY_ 0
#   endif
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#   define QE_HIDDEN_
#elif defined(__GNUC__) || defined(__clang__)
#   define QE_HIDDEN_ __attribute__((visibility("hidden")))
#else
#   define QE_HIDDEN_
#endif

#if defined(__has_attribute)
#   if __has_attribute(aligned)
#       define QE_HAS_ATTRIBUTE_ALIGNED_ 1
#   else
#       define QE_HAS_ATTRIBUTE_ALIGNED_ 0
#   endif
#else
#   if QE_COMPILER_GNUC_COMPAT_
#       define QE_HAS_ATTRIBUTE_ALIGNED_ 1
#   else
#       define QE_HAS_ATTRIBUTE_ALIGNED_ 0
#   endif
#endif

#if defined(__has_attribute)
#   if __has_attribute(always_inline)
#       define QE_HAS_ATTRIBUTE_ALWAYS_INLINE_ 1
#   else
#       define QE_HAS_ATTRIBUTE_ALWAYS_INLINE_ 0
#   endif
#else
#   if QE_COMPILER_GNUC_COMPAT_
#       define QE_HAS_ATTRIBUTE_ALWAYS_INLINE_ 1
#   else
#       define QE_HAS_ATTRIBUTE_ALWAYS_INLINE_ 0
#   endif
#endif

#if defined(__has_attribute)
#   if __has_attribute(noinline)
#       define QE_HAS_ATTRIBUTE_NOINLINE_ 1
#   else
#       define QE_HAS_ATTRIBUTE_NOINLINE_ 0
#   endif
#else
#   if QE_COMPILER_GNUC_COMPAT_
#       define QE_HAS_ATTRIBUTE_NOINLINE_ 1
#   else
#       define QE_HAS_ATTRIBUTE_NOINLINE_ 0
#   endif
#endif

#if defined(__has_builtin)
#   if __has_builtin(__builtin_unreachable)
#       define QE_HAS_BUILTIN_UNREACHABLE_ 1
#   else
#       define QE_HAS_BUILTIN_UNREACHABLE_ 0
#   endif
#else
#   if QE_COMPILER_GNUC_COMPAT_
#       define QE_HAS_BUILTIN_UNREACHABLE_ 1
#   else
#       define QE_HAS_BUILTIN_UNREACHABLE_ 0
#   endif
#endif

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#   define QE_HAS_CPP_ALIGNAS_ 1
#else
#   define QE_HAS_CPP_ALIGNAS_ 0
#endif

#if !defined(__cplusplus) && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#   define QE_HAS_C_ALIGNAS_ 1
#else
#   define QE_HAS_C_ALIGNAS_ 0
#endif

#if !defined(__cplusplus) && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#   define QE_HAS_C_RESTRICT_ 1
#else
#   define QE_HAS_C_RESTRICT_ 0
#endif

/* ------------------------------------------------------------------------- */
/* Internal ABI primitives                                                    */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
#   define QE_EXTERN_C_ extern "C"
#else
#   define QE_EXTERN_C_
#endif

#if QE_TARGET_PLATFORM_WINDOWS_ || QE_TARGET_PLATFORM_CYGWIN_
#   if defined(QE_BUILD_SHARED) && (QE_BUILD_SHARED == 1)
#       if defined(QE_BUILDING_LIBRARY) && (QE_BUILDING_LIBRARY == 1)
#           if QE_HAS_DECLSPEC_DLLEXPORT_
#               define QE_EXPORT_ __declspec(dllexport)
#           else
#               define QE_EXPORT_
#           endif
#       else
#           if QE_HAS_DECLSPEC_DLLIMPORT_
#               define QE_EXPORT_ __declspec(dllimport)
#           else
#               define QE_EXPORT_
#           endif
#       endif
#   else
#       define QE_EXPORT_
#   endif
#elif QE_HAS_ATTRIBUTE_VISIBILITY_
#   define QE_EXPORT_ __attribute__((visibility("default")))
#else
#   define QE_EXPORT_
#endif

#if defined(__clang__) && (defined(__wasm__) || defined(__wasm32__) || defined(__wasm64__))
#   define QE_IMPORT_(module, name) __attribute__((import_module(module), import_name(name)))
#else
#   define QE_IMPORT_(module, name)
#endif

#if QE_TARGET_PLATFORM_WINDOWS_ || QE_TARGET_PLATFORM_CYGWIN_
#   define QE_CALL_ __cdecl
#else
#   define QE_CALL_
#endif

/* ------------------------------------------------------------------------- */
/* Alignment helper                                                           */
/* ------------------------------------------------------------------------- */

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#   define QE_ALIGNAS_(n) alignas(n)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#   define QE_ALIGNAS_(n) _Alignas(n)
#elif defined(_MSC_VER)
#   define QE_ALIGNAS_(n) __declspec(align(n))
#elif defined(__GNUC__) || defined(__clang__)
#   define QE_ALIGNAS_(n) __attribute__((aligned(n)))
#else
#   error "QE requires alignment support"
#endif

/* ------------------------------------------------------------------------- */
/* Concatenation helpers                                                       */
/* ------------------------------------------------------------------------- */

#define QE_CONCAT_IMPL_(a, b) a##b
#define QE_CONCAT_(a, b)      QE_CONCAT_IMPL_(a, b)

/* ------------------------------------------------------------------------- */
/* Static assert helper                                                        */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
#   define QE_STATIC_ASSERT_(cond, msg) static_assert((cond), msg)
#else
#   if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#       define QE_STATIC_ASSERT_(cond, msg) _Static_assert((cond), msg)
#   else
#       define QE_STATIC_ASSERT_(cond, msg) \
        typedef char QE_CONCAT_(qe_static_assertion_, __LINE__)[(cond) ? 1 : -1]
#   endif
#endif

#endif // QE_API_MACROS_H
