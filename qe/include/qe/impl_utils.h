/*
 * C implementation utilities.
 *
 * This header is intended for C implementation units. It is not guaranteed to
 * be C++-clean. Public C/C++ ABI headers should include <qe/abi_utils.h>
 * instead.
 */

#ifndef QE_IMPL_UTILS_H
#define QE_IMPL_UTILS_H

#include <qe/internals/impl_defs.h>
#include <stdint.h>
#include <stddef.h>



typedef uint8_t qe_bool;

QE_MAYBE_UNUSED static const uint8_t qe_false = 0;
QE_MAYBE_UNUSED static const uint8_t qe_true = 1;

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

#   if QE_LITTLE_ENDIAN
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

#   if QE_LITTLE_ENDIAN
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
            qe_word16_t word16_1;
            qe_word16_t word16_0;
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

#   if QE_LITTLE_ENDIAN
        struct
        {
            qe_word32_t word32_0;
            qe_word32_t word32_1;
        };
        struct
        {
            qe_word16_t word16_0;
            qe_word16_t word16_1;
            qe_word16_t word16_2;
            qe_word16_t word16_3;
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
            qe_word16_t word16_3;
            qe_word16_t word16_2;
            qe_word16_t word16_1;
            qe_word16_t word16_0;
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

QE_MAYBE_UNUSED QE_SIC void qe_memset(void* dst, uint8_t value, size_t count)
{
    uint8_t *d = (uint8_t *)dst;
    while (count != 0u)
    {
        *d++ = value;
        --count;
    }
}

QE_MAYBE_UNUSED QE_SIC void qe_memcpy(void* QE_RESTRICT dst, const void* QE_RESTRICT src, size_t count)
{
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;

    while (count != 0u)
    {
        *d++ = *s++;
        --count;
    }
}

#endif // QE_IMPL_UTILS_H
