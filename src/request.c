#include "request.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
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
int get_start_line(char *request, size_t request_len,
                   struct request_start_line *start_line) {

  // Find first delimiter (space)
  const char *sp1 = memchr(request, ' ', request_len);
  if (!sp1) {
    return -1;
  }

  size_t method_len = (size_t)(sp1 - request);
  if (method_len >= sizeof(start_line->method)) {
    fprintf(stderr, "Method too long\n");
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
    fprintf(stderr, "URI too long\n");
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
    fprintf(stderr, "Protocol too long\n");
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

static void close_file(FILE *f) {
  if (f && fclose(f) != 0) {
    perror("Warning: could not close the file");
  }
}

off_t read_file(const char *path, char *file_buffer, size_t len) {
  FILE *fptr = NULL;
  off_t file_size;

  char file_path[MAX_FILE_PATH_LENGTH + 1];
  const char file_dir[] = "./static/";

  // Build file path
  int n = snprintf(file_path, sizeof(file_path), "%s%s", file_dir, path);

  if (n < 0) {
    fprintf(stderr, "Encoding error in snprintf\n");
    return -1;
  }

  if ((size_t)n >= sizeof(file_path)) {
    fprintf(stderr, "Path too long\n");
    return -1;
  }

  // Open file
  fptr = fopen(file_path, "rb");
  if (!fptr) {
    fprintf(stderr, "Could not open file %s: %s\n", file_path, strerror(errno));
    return -1;
  }

  // Determine file size
  if (fseeko(fptr, 0, SEEK_END) != 0) {
    perror("Could not go to file end");
    close_file(fptr);
    return -1;
  }
  file_size = ftello(fptr);
  if (file_size == -1) {
    perror("Could not verify file size");
    close_file(fptr);
    return -1;
  }
  rewind(fptr);

  // Read into file buffer
  fread(file_buffer, 1, len, fptr);
  if (ferror(fptr)) {
    perror("fread failed");
    close_file(fptr);
    return -1;
  }
  close_file(fptr);

  printf("%s\n", file_buffer);

  return file_size;
}

void handle_request(int sock) {
  char buffer[MAX_REQUEST_SIZE + 1];
  ssize_t received_size = read(sock, buffer, sizeof(buffer));
  if (received_size == -1) {
    return;
  }
  buffer[received_size] = '\0';

  struct request_start_line start_line;
  get_start_line(buffer, received_size, &start_line);

  long sent_bytes = 0;
  char response_buffer[MAX_FILE_SIZE];
  ssize_t file_size = read_file(start_line.uri, response_buffer, MAX_FILE_SIZE);

  const char *headers = create_headers(file_size);
  const size_t headers_size = strlen(headers);

  const size_t full_size = headers_size + file_size;
  char *const shared_buffer = malloc(full_size + 1);

  strcpy(shared_buffer, headers);
  strncat(shared_buffer, response_buffer, full_size + 1);

  while (1) {
    sent_bytes += send(sock, shared_buffer, full_size, 0);
    if (sent_bytes == -1) {
      perror("send");
      break;
    }
    if (sent_bytes == (full_size))
      break;
  }
  shutdown(sock, SHUT_WR);
}
