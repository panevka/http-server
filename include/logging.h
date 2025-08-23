#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>

typedef enum { MSG_WARNING, MSG_ERROR } msg_type_t;

__attribute__((format(printf, 3, 4))) void
log_msg(msg_type_t type, bool with_errno, const char *fmt, ...);

#endif
