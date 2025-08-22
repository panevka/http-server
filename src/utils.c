#include "utils.h"
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

__attribute__((format(printf, 3, 4))) void
perror_msg(msg_type_t type, bool with_errno, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  // Print severity prefix
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
