/*
 * $Id$
 */

#ifdef __GNUC__
#define LOGUTIL_PRINTF_ARG(M, N) __attribute__((__format__(__printf__, M, N)))
#else
#define LOGUTIL_PRINTF_ARG(M, N)
#endif

void logutil_message(int, const char *, ...) LOGUTIL_PRINTF_ARG(2, 3);
void logutil_error(const char *, ...) LOGUTIL_PRINTF_ARG(1, 2);
void logutil_warning(const char *, ...) LOGUTIL_PRINTF_ARG(1, 2);
void logutil_notice(const char *, ...) LOGUTIL_PRINTF_ARG(1, 2);
void logutil_info(const char *, ...) LOGUTIL_PRINTF_ARG(1, 2);
void logutil_debug(const char *, ...) LOGUTIL_PRINTF_ARG(1, 2);
void logutil_warning_errno(const char *, ...) LOGUTIL_PRINTF_ARG(1, 2);

void logutil_fatal(const char *, ...) LOGUTIL_PRINTF_ARG(1, 2);
void logutil_fatal_errno(const char *, ...) LOGUTIL_PRINTF_ARG(1, 2);

void logutil_syslog_open(const char *, int, int);
