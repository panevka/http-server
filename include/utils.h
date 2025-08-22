#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
typedef enum { MSG_WARNING, MSG_ERROR } msg_type_t;

__attribute__((format(printf, 3, 4))) void
perror_msg(msg_type_t type, bool with_errno, const char *fmt, ...);

#endif
