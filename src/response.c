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

int create_response_headers(char *headers_buf, size_t buf_len,
                            size_t body_length) {
  const char headers[] = "Content-Type: text/html\r\n"
                         "Content-Length: %lu\r\n"
                         "Connection: close\r\n\r\n";
  int bytes_written = snprintf(headers_buf, buf_len, headers, body_length);

  return bytes_written;
}

void set_response_status_line(struct response *r, char *protocol,
                              char *status_code, char *reason_phrase) {
  r->status_line.protocol = protocol;
  r->status_line.status_code = status_code;
  r->status_line.reason_phrase = reason_phrase;
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

  int bytes_written =
      snprintf(base_path, MAX_FILE_PATH_LENGTH, "%s%s", cwd, base_dir);
  if (bytes_written < 0) {
    log_msg(MSG_ERROR, true,
            "could not write cwd (%s) combined with base_dir (%s) to a buffer",
            cwd, base_dir);
    return -1;
  }
  if (bytes_written >= MAX_FILE_PATH_LENGTH) {
    log_msg(
        MSG_WARNING, false,
        "path containing cwd (%s) and base_dir (%s) had to be truncated: %s",
        cwd, base_dir, base_path);
    return -1;
  }

  char sanitized_path[MAX_FILE_PATH_LENGTH + 1];
  int is_sanitized = sanitize_path(base_path, start_line->uri, sanitized_path);
  if (is_sanitized != 0) {
    log_msg(MSG_WARNING, false, "could not properly sanitize path %s",
            base_path);
    return -1;
  }

  int file_fd = get_file_fd(sanitized_path);
  if (file_fd < 0) {
    log_msg(MSG_ERROR, true,
            "could not get file descriptor from received path %s",
            sanitized_path);
    return -1;
  }

  struct stat st;
  if (fstat(file_fd, &st) == -1) {
    log_msg(MSG_ERROR, true, "fstat has failed");
    return -1;
  }

  char *protocol = "HTTP/1.1";
  char *status_code = "200";
  char *reason_phrase = "OK";
  char headers[512];

  int headers_size =
      create_response_headers(headers, sizeof(headers), st.st_size);
  if (headers_size < 0) {
    log_msg(MSG_ERROR, true, "could not create headers");
    return -1;
  }
  if (headers_size >= sizeof(headers)) {
    log_msg(MSG_WARNING, false, "headers had to be truncated");
    return -1;
  }

  set_response_status_line(response, protocol, status_code, reason_phrase);
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
