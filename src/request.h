#ifndef REQUEST_H
#define REQUEST_H
#include <stddef.h>
#include <stdlib.h>

#define MAX_METHOD_LENGTH 3
#define MAX_URI_LENGTH 8000
#define MAX_PROTOCOL_LENGTH 20
#define MAX_REQUEST_SIZE 8192

#define MAX_FILE_PATH_LENGTH 4096
#define MAX_FILE_SIZE 1048576

struct request_start_line {
  char method[MAX_METHOD_LENGTH + 1];
  char uri[MAX_URI_LENGTH + 1];
  char protocol[MAX_PROTOCOL_LENGTH + 1];
};

int get_start_line(char *request, size_t request_len,
                   struct request_start_line *start_line);

void handle_request(int sock);

char *create_headers(size_t body_length);

off_t read_file(const char *path, char *file_buffer, size_t len);

#endif
