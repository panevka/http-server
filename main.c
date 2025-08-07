#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define LISTEN_BACKLOG 10
#define PORT "3000"
#define PROTOCOL_NAME "tcp"

int main(void) {
  int sock, new_sock; // socket
  struct sockaddr_storage new_addr;
  socklen_t addr_size;

  int binding;
  struct protoent *protocolptr; // pointer to a protocol
  struct addrinfo hints, *res, *p;

  char *msg = "wazzup";
  int len, bytes_sent;

  int rv;
  int yes = 1;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // fill ip automatically

  if ((rv = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for (p = res; p != NULL; p = p->ai_next) {
    if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
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
    exit(1);
  }

  if (listen(sock, LISTEN_BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  printf("server: waiting for connections...\n");

 while(1) {  // main accept() loop
        addr_size = sizeof new_addr;
        new_sock = accept(sock, (struct sockaddr *)&new_addr,
            &addr_size);
        if (new_sock == -1) {
            perror("accept");
            continue;
        }

        // inet_ntop(new_addr.ss_family,
        //     get_in_addr((struct sockaddr *)&new_addr),
        //     s, sizeof s);
        // printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sock); // child doesn't need the listener
            if (send(new_sock, "Hello, world!", 13, 0) == -1)
                perror("send");
            close(new_sock);
            exit(0);
        }
        close(new_sock);  // parent doesn't need this
    }

  return EXIT_SUCCESS;
}
