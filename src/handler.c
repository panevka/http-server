#include "io.h"
#include "logging.h"
#include "request.h"
#include "utils.h"
#include <libgen.h>
#include <stdio.h>
#include <unistd.h>

int handle_post_request(struct request *req) {

  FILE *fptr = NULL;
  char base_dir[] = "/static";
  char base_path[MAX_FILE_PATH_LENGTH + 1];
  char cwd[MAX_FILE_PATH_LENGTH + 1];
  char sanitized_path[MAX_FILE_PATH_LENGTH + 1];
  // size_t bytes_written_to_a_file = 0;
  // char *mime_type;

  int return_code = -1;
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    log_msg(MSG_ERROR, true, "Could not get current working directory.");
    goto cleanup;
  }

  int bytes_written =
      snprintf(base_path, MAX_FILE_PATH_LENGTH, "%s%s", cwd, base_dir);
  if (bytes_written < 0) {
    log_msg(MSG_ERROR, true,
            "could not write cwd (%s) combined with base_dir (%s) to a buffer",
            cwd, base_dir);
    goto cleanup;
  }

  if (bytes_written >= MAX_FILE_PATH_LENGTH) {
    log_msg(
        MSG_WARNING, false,
        "path containing cwd (%s) and base_dir (%s) had to be truncated: %s",
        cwd, base_dir, base_path);
    goto cleanup;
  }

  int is_sanitized =
      sanitize_path(base_path, req->start_line.uri, sanitized_path);
  if (is_sanitized != 0) {
    log_msg(MSG_WARNING, false, "could not properly sanitize path %s%s",
            base_path, req->start_line.uri);
    goto cleanup;
  }

  fptr = fopen(sanitized_path, "w");
  if (fptr == NULL) {
    log_msg(MSG_ERROR, true, "could not open file in path %s", sanitized_path);
    goto cleanup;
  }

  if (fwrite(req->body, sizeof(char), req->body_size, fptr) != req->body_size) {
    log_msg(MSG_ERROR, true,
            "could not write all data from buffer to a file at path: %s",
            sanitized_path);
  } else {
    log_msg(MSG_INFO, true, "successfully written all bytes to %s",
            sanitized_path);
  }

  return_code = 0;

cleanup:
  return return_code;
}
