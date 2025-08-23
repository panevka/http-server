#include "logging.h"
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int create_and_bind(char *port) {

  int sock;
  struct addrinfo hints, *res, *p;

  int rv;
  int yes = 1;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // fill ip automatically

  if ((rv = getaddrinfo(NULL, port, &hints, &res)) != 0) {
    log_msg(MSG_ERROR, false, "getaddrinfo: %s\n", gai_strerror(rv));
    return -1;
  }

  for (p = res; p != NULL; p = p->ai_next) {
    if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      log_msg(MSG_ERROR, true, "socket");
      continue;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      log_msg(MSG_ERROR, true, "setsockopt failed");
      return -1;
    }

    if (bind(sock, p->ai_addr, p->ai_addrlen) == -1) {
      close(sock);
      log_msg(MSG_ERROR, true, "server bind failed");
      continue;
    }

    break;
  }

  freeaddrinfo(res);

  if (p == NULL) {
    return -1;
  }

  return sock;
}
