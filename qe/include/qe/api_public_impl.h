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

#ifndef QE_API_PUBLIC_IMPL_H__
#define QE_API_PUBLIC_IMPL_H__

#ifdef __cplusplus
#   define QE_EXTERN_C_ extern "C"
#else
#   define QE_EXTERN_C_
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#   if defined(QE_BUILD_SHARED) && (QE_BUILD_SHARED == 1)
#      if defined(QE_BUILDING_LIBRARY) && (QE_BUILDING_LIBRARY == 1)
#          define QE_EXPORT_ __declspec(dllexport)
#      else
#          define QE_EXPORT_ __declspec(dllimport)
#      endif
#   else
#      define QE_EXPORT_
#   endif
#elif defined(__GNUC__) || defined(__clang__)
#   define QE_EXPORT_ __attribute__((visibility("default")))
#else
#   define QE_EXPORT_
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#   define QE_CALL_ __cdecl
#else
#   define QE_CALL_
#endif

#if defined(__cplusplus)
#  define QE_ALIGNAS_(n) alignas(n)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define QE_ALIGNAS_(n) _Alignas(n)
#elif defined(_MSC_VER)
#  define QE_ALIGNAS_(n) __declspec(align(n))
#elif defined(__GNUC__) || defined(__clang__)
#  define QE_ALIGNAS_(n) __attribute__((aligned(n)))
#else
#  error "QE requires alignment support"
#endif

#endif // QE_API_PUBLIC_IMPL_H__
