#include "logging.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

__attribute__((format(printf, 3, 4))) void
log_msg(msg_type_t type, bool with_errno, const char *fmt, ...) {

  va_list args;
  va_start(args, fmt);

  char without_ms[64];
  char with_ms[64];
  struct timeval tv;
  struct tm *tm;

  gettimeofday(&tv, NULL);
  if ((tm = localtime(&tv.tv_sec)) != NULL) {
    strftime(without_ms, sizeof(without_ms), "%Y-%m-%d %H:%M:%S.%%06u %z", tm);
    snprintf(with_ms, sizeof(with_ms), without_ms, tv.tv_usec);
    fprintf(stdout, "[%s] ", with_ms);
  }

  // Print severity prefix
  if (type == MSG_INFO)
    fprintf(stdout, "Info: ");
  if (type == MSG_WARNING)
    fprintf(stderr, "Warning: ");
  else if (type == MSG_ERROR)
    fprintf(stderr, "Error: ");

  // Print the user's formatted message
  vfprintf(stderr, fmt, args);
  va_end(args);

  // Optionally append strerror(errno)
  if (with_errno) {
    fprintf(stderr, ": %s", strerror(errno));
  }

  fprintf(stderr, "\n");
}
