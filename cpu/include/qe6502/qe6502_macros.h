/*
 * MIT License
 *
 * Copyright (c) 2025 Nikolay Nedelchev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 */

#ifndef QE6502_MACROS_H__
#define QE6502_MACROS_H__

#ifdef __cplusplus
#   define QE_EXTERN_C extern "C"
#else
#   define QE_EXTERN_C
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#   if defined(QE6502_BUILD_SHARED) && (QE6502_BUILD_SHARED == 1)
#      if QE6502_BUILDING_LIBRARY
#          define QE_EXPORT __declspec(dllexport)
#      else
#          define QE_EXPORT __declspec(dllimport)
#      endif
#   else
#      define QE_EXPORT
#   endif
#elif defined(__GNUC__) || defined(__clang__)
#   define QE_EXPORT __attribute__((visibility("default")))
#else
#   define QE_EXPORT
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#   define QE_CALL __cdecl
#else
#   define QE_CALL
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#   define QE_HIDDEN
#elif defined(__GNUC__) || defined(__clang__)
#   define QE_HIDDEN __attribute__((visibility("hidden")))
#else
#   define QE_HIDDEN
#endif

#define QE_FFI_API(rettype) QE_EXTERN_C QE_EXPORT rettype QE_CALL

#if defined(__cplusplus)
#  define QE_ALIGNAS(n) alignas(n)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define QE_ALIGNAS(n) _Alignas(n)
#elif defined(_MSC_VER)
#  define QE_ALIGNAS(n) __declspec(align(n))
#elif defined(__GNUC__) || defined(__clang__)
#  define QE_ALIGNAS(n) __attribute__((aligned(n)))
#else
#  error "qe6502 requires alignment support"
#endif

#endif // QE6502_MACROS_H__
