#ifndef UTILS_H
#define UTILS_H

#include "io.h"
#include <stdbool.h>

int starts_with(const char *string, const char *prefix);

int sanitize_path(const char *safe_base_path, const char *unsafe_path,
                  char sanitized_path[MAX_FILE_PATH_LENGTH + 1]);
#endif
