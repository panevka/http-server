#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>

#define PROTOCOL_NAME "tcp"

int main(void) {
  int sock; // socket
  struct protoent* protocolptr; // pointer to a protocol

  protocolptr = getprotobyname(PROTOCOL_NAME);
  sock = socket(AF_INET, SOCK_STREAM, protocolptr->p_proto);

  return EXIT_SUCCESS;
}
