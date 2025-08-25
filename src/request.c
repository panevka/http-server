#include "request.h"
#include "io.h"
#include "logging.h"
#include "utils.h"
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

#define MAX_METHOD_LENGTH 3
#define MAX_URI_LENGTH 8000
#define MAX_PROTOCOL_LENGTH 20
#define MAX_REQUEST_SIZE 8192

struct request_start_line {
  char method[MAX_METHOD_LENGTH + 1];
  char uri[MAX_URI_LENGTH + 1];
  char protocol[MAX_PROTOCOL_LENGTH + 1];
};

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
int get_start_line(char *request, size_t request_len,
                   struct request_start_line *start_line) {

  // Find first delimiter (space)
  const char *sp1 = memchr(request, ' ', request_len);
  if (!sp1) {
    return -1;
  }

  size_t method_len = (size_t)(sp1 - request);
  if (method_len >= sizeof(start_line->method)) {
    log_msg(MSG_ERROR, false, "method too long");
    return -1;
  }

  // Find second delimiter (space)
  const char *sp2 =
      memchr(sp1 + 1, ' ', request_len - (size_t)(sp1 + 1 - request));
  if (!sp2) {
    return -1;
  }

  size_t uri_len = (size_t)(sp2 - (sp1 + 1));
  if (uri_len >= sizeof(start_line->uri)) {
    log_msg(MSG_ERROR, false, "URI too long");
    return -1;
  }

  // Find third delimiter (carriage return and line feed)
  const char *crlf =
      memchr(sp2 + 1, '\r', request_len - (size_t)(sp2 + 1 - request));
  if (!crlf) {
    crlf = request + request_len;
  }

  size_t proto_len = (size_t)(crlf - (sp2 + 1));
  if (proto_len >= sizeof(start_line->protocol)) {
    log_msg(MSG_ERROR, false, "protocol too long");
    return -1;
  }

  // Copy fields to received struct
  memcpy(start_line->method, request, method_len);
  start_line->method[method_len] = '\0';

  memcpy(start_line->uri, sp1 + 1, uri_len);
  start_line->uri[uri_len] = '\0';

  memcpy(start_line->protocol, sp2 + 1, proto_len);
  start_line->protocol[proto_len] = '\0';

  return 0;
}

char *create_headers(size_t body_length) {
  const char headers[] = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/html\r\n"
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

  char buffer[MAX_REQUEST_SIZE + 1];
  ssize_t read_result;

  read_result = read_request(sock, buffer, sizeof(buffer) - 1);

  if (read_result == REQ_ERROR) {

    if (errno != EWOULDBLOCK && errno != EAGAIN) {
      log_msg(MSG_ERROR, true, "error occurred while reading request data");
      shutdown(sock, SHUT_WR);
      return;
    }

    log_msg(MSG_WARNING, false, "request timeout");

  } else if (read_result == REQ_OVERFLOW) {
    log_msg(MSG_WARNING, false, "could not handle request, request too big");
  }

  buffer[sizeof(buffer) - 1] = '\0';

  struct request_start_line start_line;
  get_start_line(buffer, read_result, &start_line);

  long sent_bytes = 0;

  char cwd[MAX_FILE_PATH_LENGTH + 1];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    fprintf(stderr, "Could not get current working directory: %s. \n",
            strerror(errno));
    shutdown(sock, SHUT_WR);
    return;
  }
  printf("%s", cwd);

  char base_dir[] = "/static";
  char base_path[MAX_FILE_PATH_LENGTH + 1];
  snprintf(base_path, MAX_FILE_PATH_LENGTH, "%s%s", cwd, base_dir);

  char sanitized_path[MAX_FILE_PATH_LENGTH + 1];
  int is_sanitized = sanitize_path(base_path, start_line.uri, sanitized_path);
  if (is_sanitized != 0) {
    shutdown(sock, SHUT_WR);
    return;
  }

  int file_fd = get_file_fd(sanitized_path);
  if (file_fd < 0) {
    shutdown(sock, SHUT_WR);
    return;
  }

  struct stat st;

  if (fstat(file_fd, &st) == -1) {
    log_msg(MSG_ERROR, true, "fstat has failed");
  }

  const char *headers = create_headers(st.st_size);
  const size_t headers_size = strlen(headers);

  while (1) {
    log_msg(MSG_INFO, false, "sending");
    sent_bytes += send(sock, headers, headers_size, 0);
    if (sent_bytes == -1) {
      log_msg(MSG_ERROR, true, "send has failed");
      break;
    }
    if (sent_bytes == (headers_size))
      break;
  }

  ssize_t transferred = sendfile(sock, file_fd, NULL, st.st_size);
  if (transferred < 0) {
    log_msg(MSG_ERROR, true, "transfer failed");
  }

  if (close(file_fd) != 0) {
    log_msg(MSG_WARNING, true, "closing file descriptor has failed");
  };

  shutdown(sock, SHUT_WR);
}
