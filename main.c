#include <netdb.h>
#include <stdlib.h>

int main(void) {
  int sock; // socket
  struct protoent* protocolptr; // pointer to a protocol

  protocolptr = getprotobyname(PROTOCOL_NAME);

  return EXIT_SUCCESS;
}
