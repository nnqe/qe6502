/*
 * C implementation utilities.
 *
 * This header is intended for C implementation units. It is not guaranteed to
 * be C++-clean. Public C/C++ ABI headers should include <qe/abi_utils.h>
 * instead.
 */

#ifndef QE_LOG_H
#define QE_LOG_H

#include <qe/impl_utils.h>

typedef void (QE_CALL *qe_log_fn)(void* context, const char* topic, const char* message);

QE_STATIC_API(void) qe_set_logger(qe_log_fn logger, void* context);
QE_STATIC_API(void) qe_log(const char* topic, const char *fmt, ...);
QE_STATIC_API(void) qe_log_warn(const char *fmt, ...);
QE_STATIC_API(void) qe_log_error(const char *fmt, ...);

#if defined(QE_DEBUG_BUILD) && (QE_DEBUG_BUILD == 1)
    QE_STATIC_API(void) qe_log_info(const char *fmt, ...);
#else
#   define qe_log_info(...) ((void)0)
#endif

#endif // QE_LOG_H
