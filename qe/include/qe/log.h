#ifndef QE_LOG_H__
#define QE_LOG_H__

#include <qe/api_private.h>

typedef void (*qe_log_fn)(void* context, const char* topic, const char* message);

QE_API(void) qe_set_logger(qe_log_fn logger, void* context);

QE_API(void) qe_log(const char* topic, const char *fmt, ...);
QE_API(void) qe_log_warn(const char *fmt, ...);
QE_API(void) qe_log_error(const char *fmt, ...);

#if defined(QE_DEBUG_BUILD) && (QE_DEBUG_BUILD == 1)
    QE_API(void) qe_log_info(const char *fmt, ...);
#else
#   define qe_log_info(...) ((void)0)
#endif

#endif // QE_LOG_H__
