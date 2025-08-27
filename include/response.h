#ifndef RESPONSE_H
#define RESPONSE_H

#include "request.h"
#include <sys/types.h>

#define MAX_STATUS_LINE_LENGTH 512

struct response_status_line {
  char *protocol;
  char *status_code;
  char *reason_phrase;
};

struct response {
  struct response_status_line status_line;

  char *headers;

  struct {
    int fd;
    off_t offset;
    size_t length;
  } body;
};

int prepare_response(struct response *response,
                     struct request_start_line *start_line);

int send_response(struct response *res, int sock);

#endif
