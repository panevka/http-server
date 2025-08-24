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
    log_msg(MSG_ERROR, true, "failed to create and bind socket.");
    return EXIT_FAILURE;
  }

  if (listen(socket_fd, LISTEN_BACKLOG) == -1) {
    log_msg(MSG_ERROR, true, "failed to begin listening on the socket.");
    return EXIT_FAILURE;
  }

  log_msg(MSG_INFO, false, "server: waiting for connections...");

  while (1) { // main accept() loop
    addr_size = sizeof new_addr;
    new_socket = accept(socket_fd, (struct sockaddr *)&new_addr, &addr_size);
    if (new_socket == -1) {
      log_msg(MSG_ERROR, true, "accept failed");
      continue;
    }

    if (!fork()) {      // this is the child process
      close(socket_fd); // child doesn't need the listener

      int opt_result;
      struct timeval timeout;
      timeout.tv_sec = 10;
      timeout.tv_usec = 0;

      opt_result = setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                              sizeof timeout);

      if (opt_result == 0) {
        handle_request(new_socket);
      } else if (opt_result < 0) {
        log_msg(MSG_ERROR, true, "setsockopt has failed");
      }

      close(new_socket);
      exit(0);
    }
    close(new_socket); // parent doesn't need this
  }

  return EXIT_SUCCESS;
}
