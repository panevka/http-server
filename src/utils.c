#include "utils.h"
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int starts_with(const char *string, const char *prefix) {
  while (*prefix) {
    if (*prefix++ != *string++)
      return 0;
  }

  return 1;
}

int sanitize_path(const char *safe_base_path, const char *unsafe_path,
                  char sanitized_path[MAX_FILE_PATH_LENGTH + 1]) {

  char temp[strlen(safe_base_path) + strlen(unsafe_path) + 1];
  char resolved_path[MAX_FILE_PATH_LENGTH + 1];

  snprintf(temp, MAX_FILE_PATH_LENGTH + 1, "%s%s", safe_base_path, unsafe_path);

  if (realpath(temp, resolved_path) == NULL) {
    fprintf(stderr, "Received path could not be resolved: %s. %s \n", temp,
            strerror(errno));
    return -1;
  }

  if (starts_with(resolved_path, safe_base_path)) {
    memcpy(sanitized_path, resolved_path, MAX_FILE_PATH_LENGTH);
    return 0;
  }

  return -1;
}
