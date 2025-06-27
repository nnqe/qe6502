#include <qe_utils.h>

#if (QE_ENABLE_DEBUG_LOG == 1)
    #include <stdio.h>
    #include <stdarg.h>
    #include <time.h>

    void qe_log(const char* topic, const char *fmt, ...)
    {
        /* HH:MM:SS timestamp */
        char timeBuf[9];
        time_t now = time(NULL);
        struct tm tm_now;

    #ifdef _MSC_VER
        localtime_s(&tm_now, &now);
    #else
        localtime_r(&now, &tm_now);
    #endif
        strftime(timeBuf, sizeof timeBuf, "%H:%M:%S", &tm_now);

        printf("[%s | %s] ", timeBuf, topic);

        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);

        putchar('\n');
        fflush(stdout);
    }
#endif
