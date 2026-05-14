#include "qe6502_format.h"
#include <stddef.h>

static void qe_fmt_putc(char* out, size_t cap, size_t* len, char ch)
{
    if ((*len + 1u) < cap) {
        out[*len] = ch;
    }

    *len += 1u;
}

static void qe_fmt_puts(char* out, size_t cap, size_t* len, const char* str)
{
    if (str == NULL) {
        str = "(null)";
    }

    while (*str != '\0') {
        qe_fmt_putc(out, cap, len, *str);
        str++;
    }
}

static qe_bool qe_fmt_is_digit(char ch)
{
    return ch >= '0' && ch <= '9';
}

static void qe_fmt_uint(
    char* out,
    size_t cap,
    size_t* len,
    uint64_t value,
    unsigned base,
    qe_bool uppercase,
    unsigned width,
    qe_bool zero_pad)
{
    char tmp[32];
    unsigned count = 0u;

    const char* digits = uppercase
        ? "0123456789ABCDEF"
        : "0123456789abcdef";

    if (base < 2u || base > 16u) {
        return;
    }

    if (value == 0u) {
        tmp[count++] = '0';
    } else {
        while (value != 0u && count < (unsigned)sizeof tmp) {
            const unsigned digit = (unsigned)(value % (uint64_t)base);
            tmp[count++] = digits[digit];
            value /= (uint64_t)base;
        }
    }

    while (width > count) {
        qe_fmt_putc(out, cap, len, zero_pad ? '0' : ' ');
        width--;
    }

    while (count > 0u) {
        count--;
        qe_fmt_putc(out, cap, len, tmp[count]);
    }
}

static void qe_fmt_int(
    char* out,
    size_t cap,
    size_t* len,
    int64_t value,
    unsigned width,
    qe_bool zero_pad)
{
    uint64_t magnitude;

    if (value < 0) {
        qe_fmt_putc(out, cap, len, '-');

        magnitude = (uint64_t)(-(value + 1)) + 1u;

        if (width > 0u) {
            width--;
        }
    } else {
        magnitude = (uint64_t)value;
    }

    qe_fmt_uint(out, cap, len, magnitude, 10u, qe_false, width, zero_pad);
}

QE_INTERNAL_API(int)
qe_vsnprintf(char* out, size_t cap, const char* fmt, va_list ap)
{
    size_t len = 0u;

    if (fmt == NULL) {
        fmt = "";
    }

    while (*fmt != '\0') {
        if (*fmt != '%') {
            qe_fmt_putc(out, cap, &len, *fmt);
            fmt++;
            continue;
        }

        fmt++;

        if (*fmt == '%') {
            qe_fmt_putc(out, cap, &len, '%');
            fmt++;
            continue;
        }

        qe_bool zero_pad = qe_false;
        unsigned width = 0u;
        unsigned long_count = 0u;

        if (*fmt == '0') {
            zero_pad = qe_true;
            fmt++;
        }

        while (qe_fmt_is_digit(*fmt)) {
            width = (width * 10u) + (unsigned)(*fmt - '0');
            fmt++;
        }

        if (*fmt == 'l') {
            long_count = 1u;
            fmt++;

            if (*fmt == 'l') {
                long_count = 2u;
                fmt++;
            }
        }

        switch (*fmt) {
            case 's': {
                const char* str = va_arg(ap, const char*);
                qe_fmt_puts(out, cap, &len, str);
                break;
            }

            case 'c': {
                const int ch = va_arg(ap, int);
                qe_fmt_putc(out, cap, &len, (char)ch);
                break;
            }

            case 'd':
            case 'i': {
                int64_t value;

                if (long_count >= 2u) {
                    value = (int64_t)va_arg(ap, long long);
                } else if (long_count == 1u) {
                    value = (int64_t)va_arg(ap, long);
                } else {
                    value = (int64_t)va_arg(ap, int);
                }

                qe_fmt_int(out, cap, &len, value, width, zero_pad);
                break;
            }

            case 'u': {
                uint64_t value;

                if (long_count >= 2u) {
                    value = (uint64_t)va_arg(ap, unsigned long long);
                } else if (long_count == 1u) {
                    value = (uint64_t)va_arg(ap, unsigned long);
                } else {
                    value = (uint64_t)va_arg(ap, unsigned int);
                }

                qe_fmt_uint(out, cap, &len, value, 10u, qe_false, width, zero_pad);
                break;
            }

            case 'x':
            case 'X': {
                uint64_t value;

                if (long_count >= 2u) {
                    value = (uint64_t)va_arg(ap, unsigned long long);
                } else if (long_count == 1u) {
                    value = (uint64_t)va_arg(ap, unsigned long);
                } else {
                    value = (uint64_t)va_arg(ap, unsigned int);
                }

                qe_fmt_uint(out, cap, &len, value, 16u, *fmt == 'X', width, zero_pad);
                break;
            }

            case 'p': {
                const void* ptr = va_arg(ap, const void*);
                uintptr_t value = (uintptr_t)ptr;

                qe_fmt_puts(out, cap, &len, "0x");
                qe_fmt_uint(
                    out,
                    cap,
                    &len,
                    (uint64_t)value,
                    16u,
                    qe_false,
                    (unsigned)(sizeof(uintptr_t) * 2u),
                    qe_true
                );
                break;
            }

            default: {
                qe_fmt_putc(out, cap, &len, '%');

                if (*fmt != '\0') {
                    qe_fmt_putc(out, cap, &len, *fmt);
                }

                break;
            }
        }

        if (*fmt != '\0') {
            fmt++;
        }
    }

    if (cap > 0u) {
        if (len < cap) {
            out[len] = '\0';
        } else {
            out[cap - 1u] = '\0';
        }
    }

    if (len > (size_t)INT32_MAX) {
        return INT32_MAX;
    }

    return (int)len;
}

QE_INTERNAL_API(int)
qe_snprintf(char* out, size_t cap, const char* fmt, ...)
{
    int result;
    va_list ap;

    va_start(ap, fmt);
    result = qe_vsnprintf(out, cap, fmt, ap);
    va_end(ap);

    return result;
}