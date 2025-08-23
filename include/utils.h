#ifndef UTILS_H
#define UTILS_H

#include "io.h"
#include <stdbool.h>
typedef enum { MSG_WARNING, MSG_ERROR } msg_type_t;

__attribute__((format(printf, 3, 4))) void
perror_msg(msg_type_t type, bool with_errno, const char *fmt, ...);

int starts_with(const char *string, const char *prefix);

int sanitize_path(const char *safe_base_path, const char *unsafe_path,
                  char sanitized_path[MAX_FILE_PATH_LENGTH + 1]);
#endif
