#include <qe/log.h>
#include <stdarg.h>

static qe_log_fn qe_external_logger = QE_NULL;
static void* qe_external_logger_context = QE_NULL;

static void qe_call_logger(const char* topic, const char* message)
{
    if (qe_external_logger)
    {
        qe_external_logger(qe_external_logger_context, topic? topic : "", message? message : "");
    }
}

static void qe_log_impl(const char* topic, const char *fmt, va_list ap)
{
    char message[256];

    const char* topic_notnull = topic? topic : "";
    int written = qe_vsnprintf(message, sizeof(message), fmt, ap);

    if (written < 0) {
        qe_call_logger(topic_notnull, "<qe_log format error>");
        return;
    }

    if ((unsigned)written >= sizeof message) {
        message[sizeof message - 4] = '.';
        message[sizeof message - 3] = '.';
        message[sizeof message - 2] = '.';
        message[sizeof message - 1] = '\0';
    }

    qe_call_logger(topic_notnull, message);
}

#define QE_LOG_IMPL(topic) { va_list ap; va_start(ap, fmt); qe_log_impl(topic, fmt, ap); va_end(ap); }

QE_STATIC_API_IMPL(void) qe_set_logger(qe_log_fn logger, void* context)    {qe_external_logger = logger; qe_external_logger_context = context;}
QE_STATIC_API_IMPL(void) qe_log(const char* topic, const char *fmt, ...)   {QE_LOG_IMPL(topic);}
QE_STATIC_API_IMPL(void) qe_log_warn(const char *fmt, ...)                 {QE_LOG_IMPL("WARNING");}
QE_STATIC_API_IMPL(void) qe_log_error(const char *fmt, ...)                {QE_LOG_IMPL("ERROR");}

#ifndef NDEBUG
    QE_STATIC_API_IMPL(void) qe_log_info(const char *fmt, ...)             {QE_LOG_IMPL("INFO");}
#endif
