#ifndef ROUTER_H
#define ROUTER_H
#include "request.h"
#include "response.h"

int route(struct request *req, struct response *res);

#endif
