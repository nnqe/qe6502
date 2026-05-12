#ifndef QE6502_CROSS_BUILD_H__
#define QE6502_CROSS_BUILD_H__


#include <qe6502/qe6502.h>
#include <stdint.h>
#include <string.h>

#if defined(_MSC_VER)
    #define QE_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    #define QE_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #define QE_BIG_ENDIAN 1
#else
    #error "Cannot determine endianness"
#endif

#ifdef QE_LITTLE_ENDIAN
    #define QE_LSB 0
    #define QE_MSB 1
#else
    #define QE_LSB 1
    #define QE_MSB 0
#endif // QE_LITTLE_ENDIAN

#if defined(_MSC_VER)
    #define QE_LIKELY(x)   (x)
    #define QE_UNLIKELY(x) (x)
#elif defined(__GNUC__) || defined(__clang__)
    #define QE_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define QE_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define QE_LIKELY(x)   (x)
    #define QE_UNLIKELY(x) (x)
#endif

#ifdef __cplusplus
    #define QE_RESTRICT
    #define QE_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
    #define QE_RESTRICT restrict

    #if defined(_MSC_VER)
        #define QE_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
    #elif defined(__GNUC__) || defined(__clang__)
        #define QE_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
    #else
        #define QE_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
    #endif
#endif

#define QE_FFI_API_IMPL(rettype) QE_FFI_API(rettype)

#if defined(__GNUC__) || defined(__clang__)
    #define QE_INTERNAL_API(rettype) rettype __attribute__((visibility("hidden"))) QE_CALL
#else
    #define QE_INTERNAL_API(rettype) rettype QE_CALL
#endif

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
    #define QE_ALWAYS_INLINE __forceinline
    #define QE_UNREACHABLE() __assume(0)
#elif defined(__GNUC__) || defined(__clang__)
    #define QE_ALWAYS_INLINE __attribute__((always_inline))
    #define QE_UNREACHABLE() __builtin_unreachable()
#else
    #define QE_ALWAYS_INLINE
    #define QE_UNREACHABLE() ((void)0)
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

#ifdef QE_LITTLE_ENDIAN
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
#else
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
#endif
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

#ifdef QE_LITTLE_ENDIAN
    struct
    {
        qe_word_t lsw;
        qe_word_t msw;
    };
#else
    struct
    {
        qe_word_t msw;
        qe_word_t lsw;
    };
#endif
} qe_word32_t;

#if defined(QE6502_ENABLE_DEBUG_LOG) && (QE6502_ENABLE_DEBUG_LOG == 1)
    QE_INTERNAL_API(void) qe_log(const char* topic, const char *fmt, ...);
#else
#define qe_log(...) (void)0
#endif // QE_ENABLE_LOG

#define QE_ARRAY_LENGTH(arr) ( sizeof(arr) / sizeof((arr)[0]) )
#define QE_COPY_ARRAY(dst, arr) qe_memcpy((dst), &((arr)[0]), sizeof(arr))

#define QE_CLEAR_OBJ(obj) qe_memset(&(obj), 0, sizeof(obj));
#define QE_COPY_OBJ(dst, src) qe_memcpy(&(dst), &(src), sizeof(dst));

QE_SIC void qe_memset(void* dst, uint8_t value, size_t count)
{
    memset(dst, value, count);
}
QE_MAYBE_UNUSED(qe_memset)

QE_SIC void qe_memcpy(void* QE_RESTRICT dst, const void* QE_RESTRICT src, size_t count)
{
    memcpy(dst, src, count);
}
QE_MAYBE_UNUSED(qe_memcpy)


#endif // QE6502_CROSS_BUILD_H__
