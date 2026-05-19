/*
 * QE public API aliases.
 *
 * This header intentionally contains no feature-detection logic. It only maps
 * internal support macros with a trailing underscore to their public names.
 * Include this after qe/api_abi.h, or keep the include below if this file is
 * used directly as qe/api.h.
 */

#ifndef QE_ABI_DEFS_H
#define QE_ABI_DEFS_H

#include <qe/internals/abi_macros.h>

/* ------------------------------------------------------------------------- */
/* Compiler / toolchain                                                       */
/* ------------------------------------------------------------------------- */

#define QE_COMPILER_CLANG              QE_COMPILER_CLANG_
#define QE_COMPILER_CLANG_CL           QE_COMPILER_CLANG_CL_
#define QE_COMPILER_MSVC               QE_COMPILER_MSVC_
#define QE_COMPILER_MSVC_COMPAT        QE_COMPILER_MSVC_COMPAT_
#define QE_COMPILER_GCC                QE_COMPILER_GCC_
#define QE_COMPILER_GNUC_COMPAT        QE_COMPILER_GNUC_COMPAT_
#define QE_COMPILER_MINGW              QE_COMPILER_MINGW_
#define QE_COMPILER_EMSCRIPTEN         QE_COMPILER_EMSCRIPTEN_
#define QE_COMPILER_TINYC              QE_COMPILER_TINYC_
#define QE_COMPILER_INTEL_LLVM         QE_COMPILER_INTEL_LLVM_
#define QE_COMPILER_INTEL_CLASSIC      QE_COMPILER_INTEL_CLASSIC_
#define QE_COMPILER_UNKNOWN            QE_COMPILER_UNKNOWN_

/* ------------------------------------------------------------------------- */
/* Target platform                                                            */
/* ------------------------------------------------------------------------- */

#define QE_TARGET_PLATFORM_WINDOWS     QE_TARGET_PLATFORM_WINDOWS_
#define QE_TARGET_PLATFORM_CYGWIN      QE_TARGET_PLATFORM_CYGWIN_
#define QE_TARGET_PLATFORM_LINUX       QE_TARGET_PLATFORM_LINUX_
#define QE_TARGET_PLATFORM_ANDROID     QE_TARGET_PLATFORM_ANDROID_
#define QE_TARGET_PLATFORM_APPLE       QE_TARGET_PLATFORM_APPLE_
#define QE_TARGET_PLATFORM_MACOS       QE_TARGET_PLATFORM_MACOS_
#define QE_TARGET_PLATFORM_IOS         QE_TARGET_PLATFORM_IOS_
#define QE_TARGET_PLATFORM_TVOS        QE_TARGET_PLATFORM_TVOS_
#define QE_TARGET_PLATFORM_WATCHOS     QE_TARGET_PLATFORM_WATCHOS_
#define QE_TARGET_PLATFORM_VISIONOS    QE_TARGET_PLATFORM_VISIONOS_
#define QE_TARGET_PLATFORM_WASM        QE_TARGET_PLATFORM_WASM_
#define QE_TARGET_PLATFORM_WASI        QE_TARGET_PLATFORM_WASI_
#define QE_TARGET_PLATFORM_FREEBSD     QE_TARGET_PLATFORM_FREEBSD_
#define QE_TARGET_PLATFORM_OPENBSD     QE_TARGET_PLATFORM_OPENBSD_
#define QE_TARGET_PLATFORM_NETBSD      QE_TARGET_PLATFORM_NETBSD_
#define QE_TARGET_PLATFORM_DRAGONFLYBSD QE_TARGET_PLATFORM_DRAGONFLYBSD_
#define QE_TARGET_PLATFORM_BSD         QE_TARGET_PLATFORM_BSD_
#define QE_TARGET_PLATFORM_SOLARIS     QE_TARGET_PLATFORM_SOLARIS_
#define QE_TARGET_PLATFORM_HAIKU       QE_TARGET_PLATFORM_HAIKU_
#define QE_TARGET_PLATFORM_POSIX       QE_TARGET_PLATFORM_POSIX_
#define QE_TARGET_PLATFORM_UNKNOWN     QE_TARGET_PLATFORM_UNKNOWN_

/* ------------------------------------------------------------------------- */
/* Target CPU / ISA                                                           */
/* ------------------------------------------------------------------------- */

#define QE_TARGET_ARCH_X86             QE_TARGET_ARCH_X86_
#define QE_TARGET_ARCH_X64             QE_TARGET_ARCH_X64_
#define QE_TARGET_ARCH_ARM             QE_TARGET_ARCH_ARM_
#define QE_TARGET_ARCH_ARM64           QE_TARGET_ARCH_ARM64_
#define QE_TARGET_ARCH_ARM64EC         QE_TARGET_ARCH_ARM64EC_
#define QE_TARGET_ARCH_WASM32          QE_TARGET_ARCH_WASM32_
#define QE_TARGET_ARCH_WASM64          QE_TARGET_ARCH_WASM64_
#define QE_TARGET_ARCH_RISCV32         QE_TARGET_ARCH_RISCV32_
#define QE_TARGET_ARCH_RISCV64         QE_TARGET_ARCH_RISCV64_
#define QE_TARGET_ARCH_PPC             QE_TARGET_ARCH_PPC_
#define QE_TARGET_ARCH_PPC64           QE_TARGET_ARCH_PPC64_
#define QE_TARGET_ARCH_MIPS            QE_TARGET_ARCH_MIPS_
#define QE_TARGET_ARCH_MIPS64          QE_TARGET_ARCH_MIPS64_
#define QE_TARGET_ARCH_32BIT           QE_TARGET_ARCH_32BIT_
#define QE_TARGET_ARCH_64BIT           QE_TARGET_ARCH_64BIT_
#define QE_TARGET_ARCH_UNKNOWN         QE_TARGET_ARCH_UNKNOWN_

/* ------------------------------------------------------------------------- */
/* Feature helpers                                                            */
/* ------------------------------------------------------------------------- */

#define QE_HAS_DECLSPEC_DLLEXPORT      QE_HAS_DECLSPEC_DLLEXPORT_
#define QE_HAS_DECLSPEC_DLLIMPORT      QE_HAS_DECLSPEC_DLLIMPORT_
#define QE_HAS_DECLSPEC_ALIGN          QE_HAS_DECLSPEC_ALIGN_
#define QE_HAS_DECLSPEC_NOINLINE       QE_HAS_DECLSPEC_NOINLINE_
#define QE_HAS_MSVC_ASSUME             QE_HAS_MSVC_ASSUME_
#define QE_HAS_ATTRIBUTE_VISIBILITY    QE_HAS_ATTRIBUTE_VISIBILITY_
#define QE_HAS_ATTRIBUTE_ALIGNED       QE_HAS_ATTRIBUTE_ALIGNED_
#define QE_HAS_ATTRIBUTE_ALWAYS_INLINE QE_HAS_ATTRIBUTE_ALWAYS_INLINE_
#define QE_HAS_ATTRIBUTE_NOINLINE      QE_HAS_ATTRIBUTE_NOINLINE_
#define QE_HAS_BUILTIN_UNREACHABLE     QE_HAS_BUILTIN_UNREACHABLE_
#define QE_HAS_CPP_ALIGNAS             QE_HAS_CPP_ALIGNAS_
#define QE_HAS_C_ALIGNAS               QE_HAS_C_ALIGNAS_
#define QE_HAS_C_RESTRICT              QE_HAS_C_RESTRICT_

/* ------------------------------------------------------------------------- */
/* Public API macros                                                         */
/* ------------------------------------------------------------------------- */

#define QE_ALIGNAS(n)           QE_ALIGNAS_(n)
#define QE_CALL                 QE_CALL_

#endif // QE_ABI_DEFS_H
