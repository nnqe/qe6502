#ifndef QE_API_PRIVATE_IMPL_H__
#define QE_API_PRIVATE_IMPL_H__


#include <qe/api_public.h>
#include <stdint.h>
#include <stddef.h>

#if defined(__clang__) && defined(__wasm__)
#   define QE_IMPORT_(module, name) __attribute__((import_module(module), import_name(name)))
#else
#   define QE_IMPORT_(module, name)
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#   define QE_HIDDEN_
#elif defined(__GNUC__) || defined(__clang__)
#   define QE_HIDDEN_ __attribute__((visibility("hidden")))
#else
#   define QE_HIDDEN_
#endif

#if defined(_MSC_VER)
#   define QE_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#   define QE_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#   define QE_BIG_ENDIAN 1
#else
#   error "Cannot determine endianness"
#endif

#ifdef QE_LITTLE_ENDIAN
#   define QE_LSB 0
#   define QE_MSB 1
#else
#   define QE_LSB 1
#   define QE_MSB 0
#endif // QE_LITTLE_ENDIAN

#if defined(_MSC_VER)
#   define QE_LIKELY_(x)   (x)
#   define QE_UNLIKELY_(x) (x)
#elif defined(__GNUC__) || defined(__clang__)
#   define QE_LIKELY_(x)   __builtin_expect(!!(x), 1)
#   define QE_UNLIKELY_(x) __builtin_expect(!!(x), 0)
#else
#   define QE_LIKELY_(x)   (x)
#   define QE_UNLIKELY_(x) (x)
#endif

#define QE_CONCAT_IMPL(a, b) a##b
#define QE_CONCAT_STR_(a, b) QE_CONCAT_IMPL(a, b)

#ifdef __cplusplus
#   define QE_RESTRICT_
#   define QE_STATIC_ASSERT_(cond, msg) static_assert(cond, msg)
#else
#   define QE_RESTRICT_ restrict

#   if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#       define QE_STATIC_ASSERT_(cond, msg) _Static_assert(cond, msg)
#   else
#       define QE_STATIC_ASSERT_(cond, msg) \
typedef char QE_CONCAT_STR_(qe_static_assertion_, __LINE__)[(cond) ? 1 : -1]
#   endif
#endif

#if defined(_MSC_VER)
#   define QE_ALWAYS_INLINE_ __forceinline
#   define QE_UNREACHABLE_() __assume(0)
#elif defined(__GNUC__) || defined(__clang__)
#   define QE_ALWAYS_INLINE_ __attribute__((always_inline))
#   define QE_UNREACHABLE_() __builtin_unreachable()
#else
#   define QE_ALWAYS_INLINE_
#   define QE_UNREACHABLE_() ((void)0)
#endif

#define QE_MAYBE_UNUSED_(sym)                                \
QE_SIC void qeqe_##sym##___unused_unused_qe(void);      \
    QE_SIC void qeqe_##sym##___unused_implement_qe(void) {  \
        qeqe_##sym##___unused_unused_qe(); (void)&sym;      \
}                                                       \
    QE_SIC void qeqe_##sym##___unused_unused_qe(void) {     \
        qeqe_##sym##___unused_implement_qe();               \
}


#endif // QE_API_PRIVATE_IMPL_H__
