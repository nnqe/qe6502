#ifndef QE_FORMAT_H__
#define QE_FORMAT_H__

#include <stdarg.h>

#include "qe6502_cross_build.h"

QE_INTERNAL_API(int)
qe_vsnprintf(char* out, size_t cap, const char* fmt, va_list ap);

QE_INTERNAL_API(int)
qe_snprintf(char* out, size_t cap, const char* fmt, ...);

#endif
