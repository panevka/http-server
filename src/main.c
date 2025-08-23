#include "logging.h"
#include "request.h"
#include "socket.h"
#include <netdb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define LISTEN_BACKLOG 10
#define PORT "3000"

int main(void) {
  int socket_fd, new_socket;
  struct sockaddr_storage new_addr;
  socklen_t addr_size;

  if ((socket_fd = create_and_bind(PORT)) == -1) {
    log_msg(MSG_ERROR, true, "Failed to create and bind socket.");
  }

  if (listen(socket_fd, LISTEN_BACKLOG) == -1) {
    log_msg(MSG_ERROR, true, "Failed to begin listening on the socket.");
    return EXIT_FAILURE;
  }

  log_msg(MSG_INFO, false, "server: waiting for connections...");

  if (socket_fd == -1) {
    log_msg(MSG_ERROR, true, "socket initalization failed");
    exit(1);
  }

  while (1) { // main accept() loop
    addr_size = sizeof new_addr;
    new_socket = accept(socket_fd, (struct sockaddr *)&new_addr, &addr_size);
    if (new_socket == -1) {
      log_msg(MSG_ERROR, true, "accept failed");
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
