/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <pthread.h>
#include "logutil.h"

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static int log_use_syslog = 0;

static void
syslog_opened(void)
{
	pthread_mutex_lock(&log_lock);
	log_use_syslog = 1;
	pthread_mutex_unlock(&log_lock);
}

static int
is_syslog_opened(void)
{
	int use_syslog;

	pthread_mutex_lock(&log_lock);
	use_syslog = log_use_syslog;
	pthread_mutex_unlock(&log_lock);
	return (use_syslog);
}

static void
logutil_vmessage(int priority, const char *format, va_list ap)
{
	char buffer[2048];

	vsnprintf(buffer, sizeof buffer, format, ap);

	if (is_syslog_opened())
		syslog(priority, "%s", buffer);
	else
		fprintf(stderr, "%s\n", buffer);
}

void logutil_message(int priority, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	logutil_vmessage(priority, format, ap);
	va_end(ap);
}

void logutil_error(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	logutil_vmessage(LOG_ERR, format, ap);
	va_end(ap);
}

void logutil_warning(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	logutil_vmessage(LOG_WARNING, format, ap);
	va_end(ap);
}

void logutil_notice(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	logutil_vmessage(LOG_NOTICE, format, ap);
	va_end(ap);
}

void logutil_info(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	logutil_vmessage(LOG_INFO, format, ap);
	va_end(ap);
}

void logutil_debug(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	logutil_vmessage(LOG_DEBUG, format, ap);
	va_end(ap);
}

void logutil_fatal(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	logutil_vmessage(LOG_ERR, format, ap);
	va_end(ap);
	exit(2);
}

void logutil_syslog_open(const char *log_identifier, int option, int facility)
{
	openlog(log_identifier, option, facility);
	syslog_opened();
}
