#include "response.h"
#include "io.h"
#include "logging.h"
#include "request.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

char *create_response_headers(size_t body_length) {
  const char headers[] = "Content-Type: text/html\r\n"
                         "Content-Length: %lu\r\n"
                         "Connection: close\r\n\r\n";

  int needed = snprintf(NULL, 0, headers, body_length);

  if (needed < 0)
    return NULL;

  size_t size = (size_t)needed + 1;

  char *headers_buffer = malloc(size);
  if (!headers_buffer)
    return NULL;

  if (snprintf(headers_buffer, size, headers, body_length) < 0) {
    return NULL;
  }

  return headers_buffer;
}
/**
 * @brief Formats an HTTP response status line.
 *
 * Writes a status line in the format "protocol status_code reason_phrase"
 * into the provided buffer. All string parameters must be null-terminated.
 *
 * @param status_line       Buffer to write the status line into.
 * @param status_line_len   Size of the buffer in bytes.
 * @param protocol          Null-terminated HTTP protocol version (e.g.,
 * "HTTP/1.1").
 * @param status_code       Null-terminated HTTP status code (e.g., "200").
 * @param reason_phrase     Null-terminated reason phrase (e.g., "OK").
 *
 * @return Number of characters written (excluding null terminator), or -1 on
 * error.
 */
int create_response_status_line(char *status_line, size_t status_line_len,
                                char *protocol, char *status_code,
                                char *reason_phrase) {
  int bytes_written = snprintf(status_line, status_line_len, "%s %s %s",
                               protocol, status_code, reason_phrase);

  if (bytes_written < 0) {
    return -1;
  }

  return bytes_written;
}

void set_response_status_line(struct response_status_line *status_line,
                              char *protocol, char *status_code,
                              char *reason_phrase) {
  status_line->protocol = protocol;
  status_line->status_code = status_code;
  status_line->reason_phrase = reason_phrase;
}

void set_response_headers(struct response *r, char *headers) {
  r->headers = headers;
}

void set_response_body(struct response *r, int fd, off_t offset, size_t len) {
  r->body.fd = fd;
  r->body.offset = offset;
  r->body.length = len;
}

int serialize_response_status_line(struct response_status_line *line, char *buf,
                                   size_t buf_len) {
  int size = snprintf(buf, buf_len, "%s %s %s\r\n", line->protocol,
                      line->status_code, line->reason_phrase);
  return size;
}

int prepare_response(struct response *response,
                     struct request_start_line *start_line) {

  char base_dir[] = "/static";
  char base_path[MAX_FILE_PATH_LENGTH + 1];
  char cwd[MAX_FILE_PATH_LENGTH + 1];

  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    log_msg(MSG_ERROR, true, "Could not get current working directory. ");
    return -1;
  }
  snprintf(base_path, MAX_FILE_PATH_LENGTH, "%s%s", cwd, base_dir);

  char sanitized_path[MAX_FILE_PATH_LENGTH + 1];
  int is_sanitized = sanitize_path(base_path, start_line->uri, sanitized_path);
  if (is_sanitized != 0) {
    return -1;
  }

  int file_fd = get_file_fd(sanitized_path);
  if (file_fd < 0) {
    return -1;
  }

  struct stat st;
  if (fstat(file_fd, &st) == -1) {
    log_msg(MSG_ERROR, true, "fstat has failed");
  }

  char *protocol = "HTTP/1.1";
  char *status_code = "200";
  char *reason_phrase = "OK";
  char *headers = create_response_headers(st.st_size);

  set_response_status_line(&(response->status_line), protocol, status_code,
                           reason_phrase);
  set_response_headers(response, headers);
  set_response_body(response, file_fd, 0, st.st_size);

  return 0;
}

void send_response(struct response *res, int sock) {

  long sent_bytes = 0;

  char status_line[512];
  serialize_response_status_line(&(res->status_line), status_line,
                                 sizeof(status_line));

  const size_t headers_size = strlen(res->headers);

  while (1) {
    log_msg(MSG_INFO, false, "sending %s", status_line);
    sent_bytes += send(sock, status_line, strlen(status_line), 0);
    if (sent_bytes == -1) {
      log_msg(MSG_ERROR, true, "send has failed");
      break;
    }
    if (sent_bytes == strlen(status_line))
      break;
  };
  sent_bytes = 0;

  while (1) {
    log_msg(MSG_INFO, false, "sending %s", res->headers);
    sent_bytes += send(sock, res->headers, headers_size, 0);
    if (sent_bytes == -1) {
      log_msg(MSG_ERROR, true, "send has failed");
      break;
    }
    if (sent_bytes == (headers_size))
      break;
  }

  ssize_t transferred = sendfile(sock, res->body.fd, NULL, res->body.length);
  if (transferred < 0) {
    log_msg(MSG_ERROR, true, "transfer failed");
  }

  if (close(res->body.fd) != 0) {
    log_msg(MSG_WARNING, true, "closing file descriptor has failed");
  };
}
