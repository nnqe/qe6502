#ifndef QE6502_CROSS_BUILD_H__
#define QE6502_CROSS_BUILD_H__


#include <qe6502/qe6502_macros.h>
#include <stdint.h>
#include <stddef.h>

#if defined(__clang__) && defined(__wasm__)
#   define QE_IMPORT(module, name) __attribute__((import_module(module), import_name(name)))
#else
#   define QE_IMPORT(module, name)
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
#   define QE_LIKELY(x)   (x)
#   define QE_UNLIKELY(x) (x)
#elif defined(__GNUC__) || defined(__clang__)
#   define QE_LIKELY(x)   __builtin_expect(!!(x), 1)
#   define QE_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#   define QE_LIKELY(x)   (x)
#   define QE_UNLIKELY(x) (x)
#endif

#define QE_CONCAT_IMPL(a, b) a##b
#define QE_CONCAT(a, b) QE_CONCAT_IMPL(a, b)

#ifdef __cplusplus
#   define QE_RESTRICT
#   define QE_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#   define QE_RESTRICT restrict

#   if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#       define QE_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#   else
#       define QE_STATIC_ASSERT(cond, msg) \
            typedef char QE_CONCAT(qe_static_assertion_, __LINE__)[(cond) ? 1 : -1]
#   endif
#endif

#define QE_FFI_API_IMPL(rettype) QE_FFI_API(rettype)

#define QE_INTERNAL_API(rettype) QE_HIDDEN rettype QE_CALL

#define QE_MAYBE_UNUSED(sym)                                \
    QE_SIC void qeqe_##sym##___unused_unused_qe(void);      \
    QE_SIC void qeqe_##sym##___unused_implement_qe(void) {  \
        qeqe_##sym##___unused_unused_qe(); (void)&sym;      \
    }                                                       \
    QE_SIC void qeqe_##sym##___unused_unused_qe(void) {     \
        qeqe_##sym##___unused_implement_qe();               \
    }

#define QE_API QE_EXTERN_C

#if defined(_MSC_VER)
#   define QE_ALWAYS_INLINE __forceinline
#   define QE_UNREACHABLE() __assume(0)
#elif defined(__GNUC__) || defined(__clang__)
#   define QE_ALWAYS_INLINE __attribute__((always_inline))
#   define QE_UNREACHABLE() __builtin_unreachable()
#else
#   define QE_ALWAYS_INLINE
#   define QE_UNREACHABLE() ((void)0)
#endif

#define QE_SIC static inline QE_ALWAYS_INLINE // static inline constexpr
#define QE_NULL ((void*)0)

typedef uint8_t qe_bool;

static const uint8_t qe_false = 0;
QE_MAYBE_UNUSED(qe_false)

static const uint8_t qe_true = 1;
QE_MAYBE_UNUSED(qe_true)

#define QE_U8(x) ((uint8_t)(x))
#define QE_S8(x) ((int8_t)(x))
#define QE_U16(x) ((uint16_t)(x))
#define QE_S16(x) ((int16_t)(x))
#define QE_U32(x) ((uint32_t)(x))
#define QE_S32(x) ((int32_t)(x))
#define QE_F32(x) ((float)(x))

typedef union
{
    uint16_t u16;
    int16_t i16;
    uint8_t uraw8[2];
    int8_t iraw8[2];

#   ifdef QE_LITTLE_ENDIAN
        struct
        {
            uint8_t u8_lsb;
            uint8_t u8_msb;
        };
        struct
        {
            int8_t i8_lsb;
            int8_t i8_msb;
        };
#   else
        struct
        {
            uint8_t u8_msb;
            uint8_t u8_lsb;
        };
        struct
        {
            int8_t i8_msb;
            int8_t i8_lsb;
        };
#   endif
} qe_word_t;

typedef union
{
    uint8_t u8;
    int8_t i8;
} qe_byte_t;

typedef union
{
    uint32_t u32;
    int32_t i32;
    uint8_t uraw8[4];
    int8_t iraw8[4];

#   ifdef QE_LITTLE_ENDIAN
        struct
        {
            qe_word_t lsw;
            qe_word_t msw;
        };
#   else
        struct
        {
            qe_word_t msw;
            qe_word_t lsw;
        };
#   endif
} qe_word32_t;

#define QE_ARRAY_LENGTH(arr) ( sizeof(arr) / sizeof((arr)[0]) )
#define QE_COPY_ARRAY(dst, arr) qe_memcpy((dst), &((arr)[0]), sizeof(arr))

#define QE_CLEAR_OBJ(obj) qe_memset(&(obj), 0, sizeof(obj));
#define QE_COPY_OBJ(dst, src) qe_memcpy(&(dst), &(src), sizeof(dst));

QE_SIC void qe_memset(void* dst, uint8_t value, size_t count)
{
    uint8_t *d = (uint8_t *)dst;
    while (count != 0u)
    {
        *d++ = value;
        --count;
    }
}
QE_MAYBE_UNUSED(qe_memset)

QE_SIC void qe_memcpy(void* QE_RESTRICT dst, const void* QE_RESTRICT src, size_t count)
{
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;

    while (count != 0u)
    {
        *d++ = *s++;
        --count;
    }
}
QE_MAYBE_UNUSED(qe_memcpy)


#endif // QE6502_CROSS_BUILD_H__
