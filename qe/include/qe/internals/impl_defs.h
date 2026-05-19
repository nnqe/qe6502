/*
 * QE public API aliases.
 *
 * This header intentionally contains no feature-detection logic. It only maps
 * internal support macros with a trailing underscore to their public names.
 * Include this after qe/api_abi.h, or keep the include below if this file is
 * used directly as qe/api.h.
 */

#ifndef QE_IMPL_DEFS_H
#define QE_IMPL_DEFS_H

#include <qe/internals/impl_macros.h>

/* ------------------------------------------------------------------------- */
/* IMPL / declaration helpers                                                  */
/* ------------------------------------------------------------------------- */

#define QE_INLINE                           QE_INLINE_
#define QE_FORCE_INLINE                     QE_FORCE_INLINE_
#define QE_NOINLINE                         QE_NOINLINE_
#define QE_RESTRICT                         QE_RESTRICT_
#define QE_UNREACHABLE                      QE_UNREACHABLE_
#define QE_MAYBE_UNUSED                     QE_MAYBE_UNUSED_
#define QE_UNUSED(x)                        QE_UNUSED_(x)
#define QE_HIDDEN                           QE_HIDDEN_
#define QE_LIKELY(x)                        QE_LIKELY_(x)
#define QE_UNLIKELY(x)                      QE_UNLIKELY_(x)
#define QE_CONCATENATE(a, b)                QE_CONCAT_STR_(a, b)
#define QE_INTERNAL_API(rettype)            QE_HIDDEN_ rettype QE_CALL_
#define QE_INTERNAL_API_IMPL(rettype)       QE_INTERNAL_API(rettype)
#define QE_API_IMPL(rettype)                QE_API(rettype)
#define QE_FFI_API_IMPL(rettype)            QE_FFI_API(rettype)
#define QE_ALWAYS_INLINE                    QE_ALWAYS_INLINE_
#define QE_RESTRICT                         QE_RESTRICT_
#define QE_SIC                              static inline QE_ALWAYS_INLINE_
#define QE_NULL                             ((void*)0)
#define QE_STATIC_ASSERT(cond, msg)         QE_STATIC_ASSERT_(cond, msg)

/* ------------------------------------------------------------------------- */
/* Target endianness helpers                                                  */
/* ------------------------------------------------------------------------- */

#define QE_LITTLE_ENDIAN QE_LITTLE_ENDIAN_
#define QE_BIG_ENDIAN    QE_BIG_ENDIAN_
#define QE_LSB           QE_LSB_
#define QE_MSB           QE_MSB_

/* ------------------------------------------------------------------------- */
/* Branch prediction helpers                                                  */
/* ------------------------------------------------------------------------- */

#define QE_LIKELY(x)   QE_LIKELY_(x)
#define QE_UNLIKELY(x) QE_UNLIKELY_(x)

/* ------------------------------------------------------------------------- */
/* Public API macros                                                         */
/* ------------------------------------------------------------------------- */

#define QE_API(rettype)         QE_EXTERN_C_ rettype QE_CALL_

#endif // QE_IMPL_DEFS_H
