#ifndef REQUEST_H
#define REQUEST_H
#include <stddef.h>
#include <stdint.h>

#define MAX_METHOD_LENGTH 4
#define MAX_URI_LENGTH 8000
#define MAX_PROTOCOL_LENGTH 20
#define MAX_REQUEST_SIZE 8192
#define MAX_REQUEST_SIZE 8192
#define MAX_BODY_SIZE 2097152

struct request_start_line {
  char method[MAX_METHOD_LENGTH + 1];
  char uri[MAX_URI_LENGTH + 1];
  char protocol[MAX_PROTOCOL_LENGTH + 1];
};

struct request {
  struct request_start_line start_line;
  struct hashmap *headers;
  char *body;
  size_t body_size;
};

void handle_request(int sock);

#endif
