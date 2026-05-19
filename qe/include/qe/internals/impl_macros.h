/*
 * QE public API platform / ABI support.
 *
 * This file intentionally detects the TARGET platform/architecture, not the
 * build host. For example, when compiling on macOS with Emscripten for WASM,
 * QE_TARGET_PLATFORM_WASM_ and QE_TARGET_ARCH_WASM32_ should be set, not
 * QE_TARGET_PLATFORM_MACOS_.
 */

#ifndef QE_IMPL_MACROS_H
#define QE_IMPL_MACROS_H

#include <qe/internals/abi_macros.h>


/* ------------------------------------------------------------------------- */
/* Internal IMPL primitives                                                    */
/* ------------------------------------------------------------------------- */

#if defined(__cplusplus)
#   define QE_INLINE_ inline
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#   define QE_INLINE_ inline
#elif QE_COMPILER_MSVC_COMPAT_
#   define QE_INLINE_ __inline
#elif QE_COMPILER_GNUC_COMPAT_
#   define QE_INLINE_ __inline__
#else
#   define QE_INLINE_
#endif

#if QE_COMPILER_MSVC_COMPAT_
#   define QE_FORCE_INLINE_ __forceinline
#elif QE_HAS_ATTRIBUTE_ALWAYS_INLINE_
#   define QE_FORCE_INLINE_ QE_INLINE_ __attribute__((always_inline))
#else
#   define QE_FORCE_INLINE_ QE_INLINE_
#endif

#if QE_HAS_DECLSPEC_NOINLINE_
#   define QE_NOINLINE_ __declspec(noinline)
#elif QE_HAS_ATTRIBUTE_NOINLINE_
#   define QE_NOINLINE_ __attribute__((noinline))
#else
#   define QE_NOINLINE_
#endif

#if defined(__cplusplus)
#   if QE_COMPILER_MSVC_COMPAT_
#       define QE_RESTRICT_ __restrict
#   elif QE_COMPILER_GNUC_COMPAT_
#       define QE_RESTRICT_ __restrict__
#   else
#       define QE_RESTRICT_
#   endif
#elif QE_HAS_C_RESTRICT_
#   define QE_RESTRICT_ restrict
#elif QE_COMPILER_MSVC_COMPAT_
#   define QE_RESTRICT_ __restrict
#elif QE_COMPILER_GNUC_COMPAT_
#   define QE_RESTRICT_ __restrict__
#else
#   define QE_RESTRICT_
#endif

#if QE_HAS_BUILTIN_UNREACHABLE_
#   define QE_UNREACHABLE_() __builtin_unreachable()
#elif QE_HAS_MSVC_ASSUME_
#   define QE_UNREACHABLE_() __assume(0)
#else
#   define QE_UNREACHABLE_() ((void)0)
#endif

#if defined(__cplusplus) && (__cplusplus >= 201703L)
#   define QE_HAS_CPP_MAYBE_UNUSED_ 1
#else
#   define QE_HAS_CPP_MAYBE_UNUSED_ 0
#endif

#if defined(__has_cpp_attribute)
#   if __has_cpp_attribute(maybe_unused)
#       define QE_HAS_CPP_ATTRIBUTE_MAYBE_UNUSED_ 1
#   else
#       define QE_HAS_CPP_ATTRIBUTE_MAYBE_UNUSED_ 0
#   endif
#else
#   define QE_HAS_CPP_ATTRIBUTE_MAYBE_UNUSED_ 0
#endif

#if defined(__has_attribute)
#   if __has_attribute(unused)
#       define QE_HAS_ATTRIBUTE_UNUSED_ 1
#   else
#       define QE_HAS_ATTRIBUTE_UNUSED_ 0
#   endif
#elif QE_COMPILER_GNUC_COMPAT_
#   define QE_HAS_ATTRIBUTE_UNUSED_ 1
#else
#   define QE_HAS_ATTRIBUTE_UNUSED_ 0
#endif

#if QE_HAS_CPP_MAYBE_UNUSED_ || QE_HAS_CPP_ATTRIBUTE_MAYBE_UNUSED_
#   define QE_MAYBE_UNUSED_ [[maybe_unused]]
#elif QE_HAS_ATTRIBUTE_UNUSED_
#   define QE_MAYBE_UNUSED_ __attribute__((unused))
#else
#   define QE_MAYBE_UNUSED_
#endif

#if defined(_MSC_VER)
#   define QE_ALWAYS_INLINE_ __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#   define QE_ALWAYS_INLINE_ __attribute__((always_inline))
#else
#   define QE_ALWAYS_INLINE_
#endif

#define QE_UNUSED_(x) ((void)(x))

/* ------------------------------------------------------------------------- */
/* Target endianness helpers                                                  */
/* ------------------------------------------------------------------------- */

#if defined(_MSC_VER)
#   define QE_LITTLE_ENDIAN_ 1
#   define QE_BIG_ENDIAN_ 0
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#   define QE_LITTLE_ENDIAN_ 1
#   define QE_BIG_ENDIAN_ 0
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#   define QE_LITTLE_ENDIAN_ 0
#   define QE_BIG_ENDIAN_ 1
#else
#   error "QE cannot determine target endianness"
#endif

#if QE_LITTLE_ENDIAN_
#   define QE_LSB_ 0
#   define QE_MSB_ 1
#else
#   define QE_LSB_ 1
#   define QE_MSB_ 0
#endif

/* ------------------------------------------------------------------------- */
/* Branch prediction helpers                                                  */
/* ------------------------------------------------------------------------- */

#if defined(__GNUC__) || defined(__clang__)
#   define QE_LIKELY_(x)   __builtin_expect(!!(x), 1)
#   define QE_UNLIKELY_(x) __builtin_expect(!!(x), 0)
#else
#   define QE_LIKELY_(x)   (x)
#   define QE_UNLIKELY_(x) (x)
#endif

#endif // QE_IMPL_MACROS_H
