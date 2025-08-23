#include "logging.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

__attribute__((format(printf, 3, 4))) void
log_msg(msg_type_t type, bool with_errno, const char *fmt, ...) {
  int saved_errno = errno;

  va_list args;
  va_start(args, fmt);
  char log_prefix[100];
  FILE *output_stream;

  char without_ms[64];
  char with_ms[64];
  struct timeval tv;
  struct tm tm_buf;
  struct tm *tm;

  switch (type) {
  case MSG_INFO:
    snprintf(log_prefix, sizeof(log_prefix), "%s", "Info: ");
    output_stream = stdout;
    break;
  case MSG_WARNING:
    snprintf(log_prefix, sizeof(log_prefix), "%s", "Warning: ");
    output_stream = stderr;
    break;
  case MSG_ERROR:
    snprintf(log_prefix, sizeof(log_prefix), "%s", "Error: ");
    output_stream = stderr;
    break;
  default:
    snprintf(log_prefix, sizeof(log_prefix), "%s", "Unknown: ");
    output_stream = stderr;
    break;
  }

  gettimeofday(&tv, NULL);
  if ((tm = localtime_r(&tv.tv_sec, &tm_buf)) != NULL) {
    strftime(without_ms, sizeof(without_ms), "%Y-%m-%d %H:%M:%S %z", tm);
    snprintf(with_ms, sizeof(with_ms), "%s.%06ld", without_ms,
             (long)tv.tv_usec);
    fprintf(output_stream, "[%s] ", with_ms);
  }

  // Print the formatted message with prefix
  fprintf(output_stream, "%s", log_prefix);
  vfprintf(output_stream, fmt, args);
  va_end(args);

  // Optionally append strerror(errno)
  if (with_errno) {
    char errbuf[128];
    strerror_r(saved_errno, errbuf, sizeof(errbuf));
    fprintf(output_stream, " Errno: %s", errbuf);
  }

  fprintf(output_stream, "\n");
}
