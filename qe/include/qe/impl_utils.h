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
#include <stdarg.h>
#include <stdbool.h>

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

QE_STATIC_API(int) qe_vsnprintf(char* out, size_t cap, const char* fmt, va_list ap);
QE_STATIC_API(int) qe_snprintf(char* out, size_t cap, const char* fmt, ...);


/* -------------------------------------------------------------------------- */
/* byte read                                                                  */
/*                                                                            */
/* byte 0 == least significant byte                                           */
/* -------------------------------------------------------------------------- */

static inline uint8_t qe_u16_byte(uint16_t x, unsigned byte_index)
{
    QE_ASSERT(byte_index < 2);
    return (uint8_t)(x >> (byte_index * 8));
}

static inline uint8_t qe_u32_byte(uint32_t x, unsigned byte_index)
{
    QE_ASSERT(byte_index < 4);
    return (uint8_t)(x >> (byte_index * 8));
}

static inline uint8_t qe_u64_byte(uint64_t x, unsigned byte_index)
{
    QE_ASSERT(byte_index < 8);
    return (uint8_t)(x >> (byte_index * 8));
}

/* -------------------------------------------------------------------------- */
/* byte set - returns modified value                                          */
/* -------------------------------------------------------------------------- */


static inline uint16_t qe_u16_set_byte(uint16_t x, unsigned byte_index, uint8_t value)
{
    QE_ASSERT(byte_index < 2);
    unsigned shift = byte_index * 8;
    uint16_t mask = (uint16_t)((uint16_t)0xFFu << shift);
    return (uint16_t)((x & (uint16_t)~mask) | ((uint16_t)value << shift));
}

static inline uint32_t qe_u32_set_byte(uint32_t x, unsigned byte_index, uint8_t value)
{
    QE_ASSERT(byte_index < 4);
    unsigned shift = byte_index * 8;
    uint32_t mask = (uint32_t)0xFFu << shift;
    return (x & ~mask) | ((uint32_t)value << shift);
}

static inline uint64_t qe_u64_set_byte(uint64_t x, unsigned byte_index, uint8_t value)
{
    QE_ASSERT(byte_index < 8);
    unsigned shift = byte_index * 8;
    uint64_t mask = (uint64_t)0xFFu << shift;
    return (x & ~mask) | ((uint64_t)value << shift);
}

/* -------------------------------------------------------------------------- */
/* byte write - modifies value through pointer                                */
/* -------------------------------------------------------------------------- */

static inline void qe_u16_write_byte(uint16_t *x, unsigned byte_index, uint8_t value)
{
    QE_ASSERT(x != 0);
    *x = qe_u16_set_byte(*x, byte_index, value);
}

static inline void qe_u32_write_byte(uint32_t *x, unsigned byte_index, uint8_t value)
{
    QE_ASSERT(x != 0);
    *x = qe_u32_set_byte(*x, byte_index, value);
}

static inline void qe_u64_write_byte(uint64_t *x, unsigned byte_index, uint8_t value)
{
    QE_ASSERT(x != 0);
    *x = qe_u64_set_byte(*x, byte_index, value);
}


/* -------------------------------------------------------------------------- */
/* uint16 read                                                                  */
/*                                                                            */
/* byte 0 == least significant byte                                           */
/* -------------------------------------------------------------------------- */

static inline uint16_t qe_u32_u16(uint32_t x, unsigned u16_index)
{
    QE_ASSERT(u16_index < 2);
    return (uint16_t)(x >> (u16_index * 16));
}

static inline uint16_t qe_u64_u16(uint64_t x, unsigned u16_index)
{
    QE_ASSERT(u16_index < 4);
    return (uint16_t)(x >> (u16_index * 16));
}

/* -------------------------------------------------------------------------- */
/* byte set - returns modified value                                          */
/* -------------------------------------------------------------------------- */

static inline uint32_t qe_u32_set_u16(uint32_t x, unsigned u16_index, uint16_t value)
{
    QE_ASSERT(u16_index < 2);
    unsigned shift = u16_index * 16;
    uint32_t mask = (uint32_t)0xFFFFu << shift;
    return (x & ~mask) | ((uint32_t)value << shift);
}

static inline uint64_t qe_u64_set_u16(uint64_t x, unsigned u16_index, uint16_t value)
{
    QE_ASSERT(u16_index < 2);
    unsigned shift = u16_index * 16;
    uint64_t mask = (uint64_t)0xFFFFu << shift;
    return (x & ~mask) | ((uint64_t)value << shift);
}

/* -------------------------------------------------------------------------- */
/* byte write - modifies value through pointer                                */
/* -------------------------------------------------------------------------- */

static inline void qe_u32_write_u16(uint32_t *x, unsigned u16_index, uint16_t value)
{
    QE_ASSERT(x != 0);
    *x = qe_u32_set_u16(*x, u16_index, value);
}

static inline void qe_u64_write_u16(uint64_t *x, unsigned u16_index, uint16_t value)
{
    QE_ASSERT(x != 0);
    *x = qe_u64_set_u16(*x, u16_index, value);
}

/* -------------------------------------------------------------------------- */
/* bit read                                                                   */
/*                                                                            */
/* bit 0 == least significant bit                                             */
/* -------------------------------------------------------------------------- */

static inline bool qe_u8_bit(uint8_t x, unsigned bit_index)
{
    QE_ASSERT(bit_index < 8);
    return ((x >> bit_index) & 1u) != 0;
}

static inline bool qe_u16_bit(uint16_t x, unsigned bit_index)
{
    QE_ASSERT(bit_index < 16);
    return ((x >> bit_index) & 1u) != 0;
}

static inline bool qe_u32_bit(uint32_t x, unsigned bit_index)
{
    QE_ASSERT(bit_index < 32);
    return ((x >> bit_index) & 1u) != 0;
}

static inline bool qe_u64_bit(uint64_t x, unsigned bit_index)
{
    QE_ASSERT(bit_index < 64);
    return ((x >> bit_index) & 1u) != 0;
}

/* -------------------------------------------------------------------------- */
/* bit set - returns modified value                                           */
/* -------------------------------------------------------------------------- */

static inline uint8_t qe_u8_set_bit(uint8_t x, unsigned bit_index, bool value)
{
    QE_ASSERT(bit_index < 8);
    uint8_t mask = (uint8_t)(1u << bit_index);
    return value ? (uint8_t)(x | mask) : (uint8_t)(x & (uint8_t)~mask);
}

static inline uint16_t qe_u16_set_bit(uint16_t x, unsigned bit_index, bool value)
{
    QE_ASSERT(bit_index < 16);
    uint16_t mask = (uint16_t)((uint16_t)1u << bit_index);
    return value ? (uint16_t)(x | mask) : (uint16_t)(x & (uint16_t)~mask);
}

static inline uint32_t qe_u32_set_bit(uint32_t x, unsigned bit_index, bool value)
{
    QE_ASSERT(bit_index < 32);
    uint32_t mask = (uint32_t)1u << bit_index;
    return value ? (x | mask) : (x & ~mask);
}

static inline uint64_t qe_u64_set_bit(uint64_t x, unsigned bit_index, bool value)
{
    QE_ASSERT(bit_index < 64);
    uint64_t mask = (uint64_t)1u << bit_index;
    return value ? (x | mask) : (x & ~mask);
}

/* -------------------------------------------------------------------------- */
/* bit write - modifies value through pointer                                 */
/* -------------------------------------------------------------------------- */

static inline void qe_u8_write_bit(uint8_t *x, unsigned bit_index, bool value)
{
    QE_ASSERT(x != 0);
    *x = qe_u8_set_bit(*x, bit_index, value);
}

static inline void qe_u16_write_bit(uint16_t *x, unsigned bit_index, bool value)
{
    QE_ASSERT(x != 0);
    *x = qe_u16_set_bit(*x, bit_index, value);
}

static inline void qe_u32_write_bit(uint32_t *x, unsigned bit_index, bool value)
{
    QE_ASSERT(x != 0);
    *x = qe_u32_set_bit(*x, bit_index, value);
}

static inline void qe_u64_write_bit(uint64_t *x, unsigned bit_index, bool value)
{
    QE_ASSERT(x != 0);
    *x = qe_u64_set_bit(*x, bit_index, value);
}

/* -------------------------------------------------------------------------- */
/* bit toggle                                                                 */
/* -------------------------------------------------------------------------- */

static inline uint8_t qe_u8_toggle_bit(uint8_t x, unsigned bit_index)
{
    QE_ASSERT(bit_index < 8);
    return (uint8_t)(x ^ (uint8_t)(1u << bit_index));
}

static inline uint16_t qe_u16_toggle_bit(uint16_t x, unsigned bit_index)
{
    QE_ASSERT(bit_index < 16);
    return (uint16_t)(x ^ (uint16_t)((uint16_t)1u << bit_index));
}

static inline uint32_t qe_u32_toggle_bit(uint32_t x, unsigned bit_index)
{
    QE_ASSERT(bit_index < 32);
    return x ^ ((uint32_t)1u << bit_index);
}

static inline uint64_t qe_u64_toggle_bit(uint64_t x, unsigned bit_index)
{
    QE_ASSERT(bit_index < 64);
    return x ^ ((uint64_t)1u << bit_index);
}

/* -------------------------------------------------------------------------- */
/* masks                                                                      */
/*                                                                            */
/* qe_u32_mask(8, 4) -> bits 8..11                                            */
/* Invalid ranges assert in debug and return 0 in release.                    */
/* -------------------------------------------------------------------------- */

static inline uint8_t qe_u8_mask(unsigned lsb, unsigned width)
{
    QE_ASSERT(lsb < 8);
    QE_ASSERT(width <= 8);
    QE_ASSERT(width <= 8 - lsb);
    return (uint8_t)(((uint8_t)((1u << width) - 1u)) << lsb);
}

static inline uint16_t qe_u16_mask(unsigned lsb, unsigned width)
{
    QE_ASSERT(lsb < 16);
    QE_ASSERT(width <= 16);
    QE_ASSERT(width <= 16 - lsb);
    return (uint16_t)(((uint16_t)(((uint16_t)1u << width) - 1u)) << lsb);
}

static inline uint32_t qe_u32_mask(unsigned lsb, unsigned width)
{
    QE_ASSERT(lsb < 32);
    QE_ASSERT(width <= 32);
    QE_ASSERT(width <= 32 - lsb);
    return (((uint32_t)1u << width) - 1u) << lsb;
}

static inline uint64_t qe_u64_mask(unsigned lsb, unsigned width)
{
    QE_ASSERT(lsb < 64);
    QE_ASSERT(width <= 64);
    QE_ASSERT(width <= 64 - lsb);
    return (((uint64_t)1u << width) - 1u) << lsb;
}

/* -------------------------------------------------------------------------- */
/* bit field read                                                             */
/* -------------------------------------------------------------------------- */

static inline uint8_t qe_u8_bits(uint8_t x, unsigned lsb, unsigned width)
{
    uint8_t mask = qe_u8_mask(lsb, width);
    return (uint8_t)((x & mask) >> lsb);
}

static inline uint16_t qe_u16_bits(uint16_t x, unsigned lsb, unsigned width)
{
    uint16_t mask = qe_u16_mask(lsb, width);
    return (uint16_t)((x & mask) >> lsb);
}

static inline uint32_t qe_u32_bits(uint32_t x, unsigned lsb, unsigned width)
{
    uint32_t mask = qe_u32_mask(lsb, width);
    return (x & mask) >> lsb;
}

static inline uint64_t qe_u64_bits(uint64_t x, unsigned lsb, unsigned width)
{
    uint64_t mask = qe_u64_mask(lsb, width);
    return (x & mask) >> lsb;
}

/* -------------------------------------------------------------------------- */
/* bit field set - returns modified value                                     */
/* -------------------------------------------------------------------------- */

static inline uint8_t qe_u8_set_bits(uint8_t x, unsigned lsb, unsigned width, uint8_t value)
{
    uint8_t mask = qe_u8_mask(lsb, width);
    return (uint8_t)((x & (uint8_t)~mask) | (uint8_t)((uint8_t)(value << lsb) & mask));
}

static inline uint8_t qe_u8_set_mask(uint8_t x, uint8_t mask, uint8_t values)
{
     return (uint8_t)(( x & (uint8_t)~mask ) | ( values ));
}

static inline uint16_t qe_u16_set_bits(uint16_t x, unsigned lsb, unsigned width, uint16_t value)
{
    uint16_t mask = qe_u16_mask(lsb, width);
    return (uint16_t)((x & (uint16_t)~mask) | (uint16_t)((uint16_t)(value << lsb) & mask));
}

static inline uint32_t qe_u32_set_bits(uint32_t x, unsigned lsb, unsigned width, uint32_t value)
{
    uint32_t mask = qe_u32_mask(lsb, width);
    return (x & ~mask) | ((value << lsb) & mask);
}

static inline uint64_t qe_u64_set_bits(uint64_t x, unsigned lsb, unsigned width, uint64_t value)
{
    uint64_t mask = qe_u64_mask(lsb, width);
    return (x & ~mask) | ((value << lsb) & mask);
}

/* -------------------------------------------------------------------------- */
/* bit field write - modifies value through pointer                           */
/* -------------------------------------------------------------------------- */

static inline void qe_u8_write_bits(uint8_t *x, unsigned lsb, unsigned width, uint8_t value)
{
    QE_ASSERT(x != 0);
    *x = qe_u8_set_bits(*x, lsb, width, value);
}

static inline void qe_u16_write_bits(uint16_t *x, unsigned lsb, unsigned width, uint16_t value)
{
    QE_ASSERT(x != 0);
    *x = qe_u16_set_bits(*x, lsb, width, value);
}

static inline void qe_u32_write_bits(uint32_t *x, unsigned lsb, unsigned width, uint32_t value)
{
    QE_ASSERT(x != 0);
    *x = qe_u32_set_bits(*x, lsb, width, value);
}

static inline void qe_u64_write_bits(uint64_t *x, unsigned lsb, unsigned width, uint64_t value)
{
    QE_ASSERT(x != 0);
    *x = qe_u64_set_bits(*x, lsb, width, value);
}


#endif // QE_IMPL_UTILS_H
