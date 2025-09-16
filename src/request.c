#include "request.h"
#include "hashmap.h"
#include "io.h"
#include "logging.h"
#include "response.h"
#include "router.h"
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

#define MAX_REQUEST_HEADER_KEY_SIZE 512
#define MAX_REQUEST_HEADER_VALUE_SIZE 1024

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
ssize_t parse_start_line(char *buf, size_t buf_len,
                         struct request_start_line *start_line,
                         size_t *start_line_size) {

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
  const char *method_end = sp1 + 1;
  size_t buf_content_left = buf_len - (size_t)(method_end - buf);
  const char *sp2 = memchr(method_end, ' ', buf_content_left);
  if (!sp2) {
    return -1;
  }

  size_t uri_len = (size_t)(sp2 - (sp1 + 1));
  if (uri_len >= sizeof(start_line->uri)) {
    log_msg(MSG_ERROR, false, "URI too long");
    return -1;
  }

  // Find third delimiter (carriage return and line feed)
  const char *uri_end = sp2 + 1;
  const char *cr = memchr(uri_end, '\r', buf_len - (size_t)(uri_end - buf));
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

  *start_line_size = lf - buf + 1;

  return 0;
}

typedef enum {
  REQ_SUCCESS = 0,
  REQ_ERROR = -1,
  REQ_OVERFLOW = -2 // socket contains more request data than buffer size
} request_status;

// TODO: handle infinite data sending (add max limit for received data)
int read_request(int sock, char *buffer, size_t buffer_size,
                 size_t *received_message_size) {
  size_t bytes_read = 0;
  ssize_t chunk = 0;
  int return_code = REQ_SUCCESS;

  while (bytes_read < buffer_size) {
    chunk = recv(sock, buffer + bytes_read, buffer_size - bytes_read, 0);

    if (chunk < 0) {
      return_code = REQ_SUCCESS;
      goto end;
    }

    if (chunk == 0) {
      break;
    }

    bytes_read += (size_t)chunk;
  }

  if (recv(sock, (char[1]){0}, 1, MSG_PEEK) > 0) {
    return_code = REQ_OVERFLOW;
  }

  log_msg(MSG_ERROR, false, "bytes read! %zu", bytes_read);

end:
  *received_message_size = bytes_read;
  return return_code;
}

struct header {
  char key[MAX_REQUEST_HEADER_KEY_SIZE + 1];
  char value[MAX_REQUEST_HEADER_VALUE_SIZE + 1];
};

int parse_header(char *line_buf, size_t line_buf_len, struct header *header) {

  int key_value_delimiter_met = 0;
  for (size_t i = 0; i < line_buf_len; i++) {
    char c = line_buf[i];

    if (c == ':' && key_value_delimiter_met == 0) {
      key_value_delimiter_met = 1;

      char *key_start = line_buf;
      size_t key_len = i;

      char *value_start = line_buf + key_len +
                          2; // +2 to compensate for : and space before value
      size_t value_len = (line_buf + line_buf_len) - value_start;

      if (key_len >= sizeof(header->key) ||
          value_len >= sizeof(header->value)) {
        return -1;
      }
      snprintf(header->key, key_len + 1, "%s", key_start);
      snprintf(header->value, value_len + 1, "%s", value_start);
      return 0;
    }
  }
  return 0;
}

int parse_headers(char *buffer, size_t buffer_len, struct hashmap *headers,
                  size_t *headers_len) {

  int state = 0;
  size_t header_line_start_index = 0;
  size_t header_line_end_index = 0;

  for (size_t i = 0; i < buffer_len; i++) {
    char c = buffer[i];

    switch (state) {
    case 0:
      state = (c == '\r') ? 1 : 0;
      break;
    case 1:
      state = (c == '\n') ? 2 : (c == '\r') ? 1 : 0;
      break;
    case 2:
      state = (c == '\r') ? 3 : 0;
      break;
    case 3:
      if (c == '\n') {
        *headers_len = i + 1;
        return 0;
      } else if (c == 'r') {
        state = 1;
      } else {
        state = 0;
      }
      break;
    }

    if (state == 2) {
      header_line_end_index = i - 2;
      struct header hd;
      size_t line_length = header_line_end_index - header_line_start_index +
                           1; // + 1 because indexes begin from 0
      if (parse_header(&buffer[header_line_start_index], line_length, &hd) !=
          0) {
        log_msg(MSG_ERROR, false, "could not parse header");
        return -1;
      }
      char *value_ptr = strdup(hd.value);
      if (value_ptr == NULL) {
        log_msg(MSG_ERROR, false, "could not allocate space for header value");
        return -1;
      }
      hashmap_put(headers, hd.key, value_ptr);
      header_line_start_index = i + 1;
    }
  }

  return -1; // incomplete headers
}

int parse_request(struct request *req, char *buffer, size_t buffer_len) {

  int result = -1;

  size_t start_line_size;
  result =
      parse_start_line(buffer, buffer_len, &req->start_line, &start_line_size);

  size_t headers_size;
  result = parse_headers(buffer + start_line_size, buffer_len - start_line_size,
                         req->headers, &headers_size);

  if (result == -1) {
    log_msg(MSG_INFO, false, "parsing headers failed");
    return -1;
  }

  size_t request_head_size = start_line_size + headers_size;

  size_t body_size = buffer_len - request_head_size;

  log_msg(MSG_INFO, false, "request body size is: %zu", body_size);

  req->body = malloc(body_size);
  if (req->body == NULL) {
    log_msg(MSG_INFO, false,
            "malloc failed for body allocation, could not allocate %zu",
            body_size);
    return -1;
  }

  memcpy(req->body, buffer + request_head_size, body_size);
  req->body_size = body_size;

  return 0;
}

void free_header_values(void *value) {
  if (value != NULL) {
    free(value);
  }
}

void handle_request(int sock) {

  write_dir_entries_html("/home/shef/dev/projects/http-server/static",
                         "/home/shef/dev/projects/http-server/temp/index.html");

  struct request req = {.body = NULL};
  struct hashmap *headers = hashmap_create(free_header_values);
  if (headers == NULL) {
    goto end_connection;
  }
  req.headers = headers;

  size_t bytes_read = 0;

  char request_buf[MAX_REQUEST_SIZE + 1];
  struct response res = {.body.fd = -1};

  int read_result =
      read_request(sock, request_buf, sizeof(request_buf) - 1, &bytes_read);

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

  if (parse_request(&req, request_buf, bytes_read) != 0) {
    log_msg(MSG_WARNING, false, "could not parse the request");
    goto end_connection;
  }

  if (route(&req, &res) != 0) {
    log_msg(MSG_ERROR, false, "routing has failed");
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
  if (headers != NULL) {
    hashmap_destroy(headers);
  }
  if (req.body != NULL) {
    free(req.body);
  }

  shutdown(sock, SHUT_WR);
  return;
}
