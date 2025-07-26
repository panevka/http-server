#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define PORT "3490"
#define PROTOCOL_NAME "tcp"

int main(void) {
  int sock; // socket
  int binding;
  struct protoent *protocolptr; // pointer to a protocol
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // fill ip automatically

  getaddrinfo(NULL, PORT, &hints, &res);

  protocolptr = getprotobyname(PROTOCOL_NAME);
  sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  binding = bind(sock, res->ai_addr, res->ai_addrlen);
  return EXIT_SUCCESS;
}
