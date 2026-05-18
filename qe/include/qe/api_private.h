#ifndef QE_API_PRIVATE_H__
#define QE_API_PRIVATE_H__


#include <qe/api_public.h>
#include <stdint.h>
#include <stddef.h>
#include "api_private_impl.h"

#define QE_IMPORT(module, name)             QE_IMPORT_(module, name)
#define QE_HIDDEN                           QE_HIDDEN_

#define QE_LIKELY(x)                        QE_LIKELY_(x)
#define QE_UNLIKELY(x)                      QE_UNLIKELY_(x)

#define QE_CONCATENATE(a, b)                QE_CONCAT_STR_(a, b)

#define QE_INTERNAL_API(rettype)            QE_HIDDEN_ rettype QE_CALL_
#define QE_INTERNAL_API_IMPL(rettype)       QE_INTERNAL_API(rettype)

#define QE_API_IMPL(rettype)                QE_API(rettype)
#define QE_FFI_API_IMPL(rettype)            QE_FFI_API(rettype)

#define QE_MAYBE_UNUSED(sym)                QE_MAYBE_UNUSED_(sym)

#define QE_ALWAYS_INLINE                    QE_ALWAYS_INLINE_
#define QE_UNREACHABLE()                    QE_UNREACHABLE_()

#define QE_RESTRICT                         QE_RESTRICT_

#define QE_SIC                              static inline QE_ALWAYS_INLINE_ // static inline constexpr

#define QE_NULL                             ((void*)0)

#define QE_STATIC_ASSERT(cond, msg)         QE_STATIC_ASSERT_(cond, msg)


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
            int8_t i8_0;
            int8_t i8_1;
        };
        struct
        {
            uint8_t u8_0;
            uint8_t u8_1;
        };
#   else
        struct
        {
            int8_t i8_0;
            int8_t i8_1;
        };
        struct
        {
            uint8_t u8_1;
            uint8_t u8_0;
        };
#   endif
} qe_word16_t;
#define qe_as_word16(i16) (*((qe_word16_t*)(&(i16))))

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
            qe_word16_t word16_0;
            qe_word16_t word16_1;
        };
        struct
        {
            uint8_t u8_0;
            uint8_t u8_1;
            uint8_t u8_2;
            uint8_t u8_3;
        };
#   else
        struct
        {
            qe_word16_t word_1;
            qe_word16_t word_0;
        };
        struct
        {
            uint8_t u8_3;
            uint8_t u8_2;
            uint8_t u8_1;
            uint8_t u8_0;
        };
#   endif
} qe_word32_t;
#define qe_as_word32(i32) (*((qe_word32_t*)(&(i32))))


typedef union
{
    uint64_t u64;
    int64_t i64;
    uint8_t uraw8[8];
    int8_t iraw8[8];

#   ifdef QE_LITTLE_ENDIAN
        struct
        {
            qe_word32_t word32_0;
            qe_word32_t word32_1;
        };
        struct
        {
            uint8_t u8_0;
            uint8_t u8_1;
            uint8_t u8_2;
            uint8_t u8_3;
            uint8_t u8_4;
            uint8_t u8_5;
            uint8_t u8_6;
            uint8_t u8_7;
        };
#   else
        struct
        {
            qe_word32_t word32_1;
            qe_word32_t word32_0;
        };
        struct
        {
            uint8_t u8_7;
            uint8_t u8_6;
            uint8_t u8_5;
            uint8_t u8_4;
            uint8_t u8_3;
            uint8_t u8_2;
            uint8_t u8_1;
            uint8_t u8_0;
        };
#   endif
} qe_word64_t;
#define qe_as_word64(i64) (*((qe_word64_t*)(&(i64))))


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

#endif // QE_API_PRIVATE_H__
