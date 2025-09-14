#include "logging.h"
#include "request.h"
#include "response.h"
#include <string.h>

int route(struct request *req, struct response *res) {

  int return_code = -1;

  if (strcmp(req->start_line.method, "GET") == 0) {
    log_msg(MSG_INFO, false, "GET request");
    if (prepare_response(res, &(req->start_line)) == 0) {
      return_code = 0;
    }
  } else if (strcmp(req->start_line.method, "POST") == 0) {
    log_msg(MSG_INFO, false, "POST request");
  } else {
    log_msg(MSG_WARNING, false, "unknown request method");
  }

  return return_code;
}
