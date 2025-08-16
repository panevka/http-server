#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "./src/request.h"

#define LISTEN_BACKLOG 10
#define PORT "3000"
#define PROTOCOL_NAME "tcp"

int initialize_socket(void){

  int sock;
  struct addrinfo hints, *res, *p;

  int rv;
  int yes = 1;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // fill ip automatically

  if ((rv = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return -1;
  }

  for (p = res; p != NULL; p = p->ai_next) {
    if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      return -1;
    }

    if (bind(sock, p->ai_addr, p->ai_addrlen) == -1) {
      close(sock);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(res);

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    return -1;
  }

  if (listen(sock, LISTEN_BACKLOG) == -1) {
    perror("listen");
    return -1;
  }

  printf("server: waiting for connections...\n");

  return sock;
}

int main(void) {
  int socket_fd, new_socket;
  struct sockaddr_storage new_addr;
  socklen_t addr_size;

  socket_fd = initialize_socket();
  if(socket_fd == -1) {
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

    if (!fork()) {// this is the child process
      close(socket_fd); // child doesn't need the listener
      handle_request(new_socket);
      exit(0);
    }
    close(new_socket); // parent doesn't need this
  }

  return EXIT_SUCCESS;
}
