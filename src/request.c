#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct request_start_line *resolve_request_headers(char *headers, size_t len) {

  struct request_start_line *result = malloc(sizeof(*result));
  if (!result)
    return NULL;

  char *method = malloc(MAX_METHOD_LENGTH + 1);
  char *uri = malloc(MAX_URI_LENGTH + 1);
  char *protocol = malloc(MAX_PROTOCOL_LENGTH + 1);
  if (!method || !uri || !protocol) {
    free(method);
    free(uri);
    free(protocol);
    free(result);
    return NULL;
  }

  const char *sp1 = memchr(headers, ' ', len);
  if (!sp1)
    return NULL;

  size_t method_len = (size_t)(sp1 - headers);
  if (method_len > MAX_METHOD_LENGTH)
    method_len = MAX_METHOD_LENGTH;
  memcpy(method, headers, method_len);
  method[method_len] = '\0';

  const char *sp2 = memchr(sp1 + 1, ' ', len - (sp1 + 1 - headers));
  if (!sp2)
    return NULL;

  size_t uri_len = sp2 - (sp1 + 1);
  if (uri_len > MAX_URI_LENGTH)
    uri_len = MAX_URI_LENGTH;
  memcpy(uri, sp1 + 1, uri_len);
  uri[uri_len] = '\0';

  const char *crlf = memchr(sp2 + 1, '\r', len - (sp2 + 1 - headers));
  if (!crlf)
    crlf = headers + len;

  size_t proto_len = crlf - (sp2 + 1);
  if (proto_len > MAX_PROTOCOL_LENGTH)
    proto_len = MAX_PROTOCOL_LENGTH;
  memcpy(protocol, sp2 + 1, proto_len);
  protocol[proto_len] = '\0';

  printf("Method: %s\n", method);
  printf("URI: %s\n", uri);
  printf("Protocol version: %s\n", protocol);

  strncpy(result->method, method, sizeof(result->method));
  strncpy(result->uri, uri, sizeof(result->uri));
  strncpy(result->protocol, protocol, sizeof(result->protocol));

  return result;
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

ssize_t read_file(char *path, char *file_buffer, size_t len) {
  FILE *fptr;
  long file_size;

  char file_path[MAX_FILE_PATH_LENGTH + 1];
  const char file_dir[] = "./static/";
  snprintf(file_path, sizeof(file_path), "%s%s", file_dir, path);

  fptr = fopen(file_path, "rb");
  if (!fptr) {
    fprintf(stderr, "Could not open file %s: ", file_path);
    perror("");
    return -1;
  }

  if (fseek(fptr, 0, SEEK_END) == -1) {
    perror("Could not go to file end");
    return -1;
  }
  if ((file_size = ftell(fptr)) == -1) {
    perror("Could not verify file size");
    return -1;
  }
  rewind(fptr);

  unsigned long read_bytes = fread(file_buffer, sizeof(char), len, fptr);
  if (read_bytes != len) {
    perror("Could not read full file content");
    return -1;
  }
  if (fclose(fptr) != 0) {
    perror("Warning: could not close the file");
  }

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

  resolve_request_headers(buffer, received_size);

  long sent_bytes = 0;
  char response_buffer[MAX_FILE_SIZE];
  ssize_t file_size = read_file("index.html", response_buffer, MAX_FILE_SIZE);

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
