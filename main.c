#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define LISTEN_BACKLOG 10
#define PORT "3000"
#define PROTOCOL_NAME "tcp"

int main(void) {
  int sock, new_sock; // socket
  struct sockaddr_storage new_addr;
  socklen_t addr_size;

  int binding;
  struct protoent *protocolptr; // pointer to a protocol
  struct addrinfo hints, *res;

  char *msg = "wazzup";
  int len, bytes_sent;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // fill ip automatically

  getaddrinfo(NULL, PORT, &hints, &res);

  protocolptr = getprotobyname(PROTOCOL_NAME);
  sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  binding = bind(sock, res->ai_addr, res->ai_addrlen);
  connect(sock, res->ai_addr, res->ai_addrlen);
  listen(sock, LISTEN_BACKLOG);

  addr_size = sizeof new_addr;
  new_sock = accept(sock, (struct sockaddr *)&new_addr, &addr_size);

  len = strlen(msg);
  bytes_sent = send(new_sock, msg, len,  0);

  return EXIT_SUCCESS;
}
