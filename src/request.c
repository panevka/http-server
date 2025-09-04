#include "request.h"
#include "io.h"
#include "logging.h"
#include "response.h"
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @brief Parses the start line of an HTTP request.
 *
 * This function extracts the HTTP method, request URI, and protocol
 * version from a raw HTTP request buffer. It copies each component
 * into the provided `request_start_line` structure.
 *
 * The expected format of the start line is:
 *     METHOD SP URI SP PROTOCOL CRLF
 * For example:
 *     "GET /index.html HTTP/1.1\r\n"
 *
 * @param request       Pointer to the raw HTTP request buffer (without null
 * termination).
 * @param request_len   Length of the request buffer in bytes.
 * @param start_line    Pointer to a `struct request_start_line` where
 *                      the parsed method, URI, and protocol version
 *                      will be stored. Each field must have sufficient
 *                      space to hold the corresponding string including
 *                      the null terminator.
 *
 * @return int          Returns 0 on success, -1 on failure (e.g., if
 *                      delimiters are missing, or fields exceed maximum
 * length).
 *
 * @note The function ensures that the strings in `start_line` are
 *       null-terminated. It returns -1 if the method, URI, or protocol
 *       length is equal to or exceeds the size of their respective
 *       fields in the `request_start_line` structure.
 */
int parse_start_line(char *buf, size_t buf_len,
                     struct request_start_line *start_line,
                     size_t *start_line_end) {

  // Find first delimiter (space)
  const char *sp1 = memchr(buf, ' ', buf_len);
  if (!sp1) {
    return -1;
  }

  size_t method_len = (size_t)(sp1 - buf);
  if (method_len >= sizeof(start_line->method)) {
    log_msg(MSG_ERROR, false, "method too long");
    return -1;
  }

  // Find second delimiter (space)
  const char *sp2 = memchr(sp1 + 1, ' ', buf_len - (size_t)(sp1 + 1 - buf));
  if (!sp2) {
    return -1;
  }

  size_t uri_len = (size_t)(sp2 - (sp1 + 1));
  if (uri_len >= sizeof(start_line->uri)) {
    log_msg(MSG_ERROR, false, "URI too long");
    return -1;
  }

  // Find third delimiter (carriage return and line feed)
  const char *cr = memchr(sp2 + 1, '\r', buf_len - (size_t)(sp2 + 1 - buf));
  if (!cr || cr == buf + buf_len) {
    return -1;
  }
  const char *lf = cr + 1;
  if (*lf != '\n') {
    return -1;
  }

  size_t proto_len = (size_t)(cr - (sp2 + 1));
  if (proto_len >= sizeof(start_line->protocol)) {
    log_msg(MSG_ERROR, false, "protocol too long");
    return -1;
  }

  // Copy fields to received struct
  memcpy(start_line->method, buf, method_len);
  start_line->method[method_len] = '\0';

  memcpy(start_line->uri, sp1 + 1, uri_len);
  start_line->uri[uri_len] = '\0';

  memcpy(start_line->protocol, sp2 + 1, proto_len);
  start_line->protocol[proto_len] = '\0';

  *start_line_end = (size_t)(lf - buf + 1); // buffer offset just past \n

  return 0;
}

typedef enum {
  REQ_ERROR = -1,
  REQ_OVERFLOW = -2 // socket contains more request data than buffer size
} request_status;

ssize_t read_request(int sock, char *buffer, size_t buffer_size) {
  size_t bytes_read = 0;
  ssize_t chunk = 0;

  while (bytes_read < buffer_size) {
    chunk = recv(sock, buffer + bytes_read, buffer_size - bytes_read, 0);

    if (chunk < 0) {
      return REQ_ERROR;
    }

    if (chunk == 0) {
      break;
    }

    bytes_read += (size_t)chunk;
  }

  if (recv(sock, (char[1]){0}, 1, MSG_PEEK) > 0) {
    return REQ_OVERFLOW;
  }

  return (ssize_t)bytes_read;
}

void handle_request(int sock) {

  write_dir_entries_html("/home/shef/dev/projects/http-server/static",
                         "/home/shef/dev/projects/http-server/temp/index.html");

  char request_buf[MAX_REQUEST_SIZE + 1];
  struct response res = {.body.fd = -1};
  struct request_start_line start_line;

  ssize_t read_result =
      read_request(sock, request_buf, sizeof(request_buf) - 1);

  if (read_result == REQ_ERROR) {

    if (errno != EWOULDBLOCK && errno != EAGAIN) {
      log_msg(MSG_ERROR, true, "error occurred while reading request data");
      goto end_connection;
    }

    log_msg(MSG_WARNING, false, "request timeout");

  } else if (read_result == REQ_OVERFLOW) {
    log_msg(MSG_WARNING, false, "could not handle request, request too big");
    goto end_connection;
  }

  request_buf[sizeof(request_buf) - 1] = '\0';

  get_start_line(request_buf, read_result, &start_line);

  int response_success = prepare_response(&res, &start_line);
  if (response_success != 0) {
    log_msg(MSG_ERROR, false, "could not prepare response");
    goto end_connection;
  }

  if (send_response(&res, sock) != 0) {
    log_msg(MSG_ERROR, false, "could not send response");
    goto end_connection;
  }

end_connection:
  if (res.body.fd >= 0) {
    if (close(res.body.fd) != 0) {
      log_msg(MSG_WARNING, true, "closing file descriptor has failed");
    };
    res.body.fd = -1;
  }

  shutdown(sock, SHUT_WR);
  return;
}
