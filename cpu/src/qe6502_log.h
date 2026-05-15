#ifndef QE6502_LOG_H__
#define QE6502_LOG_H__

#if defined(QE6502_ENABLE_DEBUG_LOG) && (QE6502_ENABLE_DEBUG_LOG == 1)
#   include "qe6502_cross_build.h"
    QE_INTERNAL_API(void) qe_log(const char* topic, const char *fmt, ...);

    QE_INTERNAL_API(void) qe_log_info(const char *fmt, ...);
    QE_INTERNAL_API(void) qe_log_warn(const char *fmt, ...);
    QE_INTERNAL_API(void) qe_log_error(const char *fmt, ...);
#else
#   define qe_log(...) (void)0
#   define qe_log_info(...) (void)0
#   define qe_log_warn(...) (void)0
#   define qe_log_error(...) (void)0
#endif // QE_ENABLE_LOG

#endif // QE6502_LOG_H__
