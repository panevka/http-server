#include "request.h"
#include "socket.h"
#include <netdb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void) {
  int socket_fd, new_socket;
  struct sockaddr_storage new_addr;
  socklen_t addr_size;

  socket_fd = initialize_socket();
  if (socket_fd == -1) {
    perror("socket initalization failed");
    exit(1);
  }

  while (1) { // main accept() loop
    addr_size = sizeof new_addr;
    new_socket = accept(socket_fd, (struct sockaddr *)&new_addr, &addr_size);
    if (new_socket == -1) {
      perror("accept");
      continue;
    }

    if (!fork()) {      // this is the child process
      close(socket_fd); // child doesn't need the listener
      handle_request(new_socket);
      close(new_socket);
      exit(0);
    }
    close(new_socket); // parent doesn't need this
  }

  return EXIT_SUCCESS;
}
