#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define LISTEN_BACKLOG 10
#define PORT "3000"
#define PROTOCOL_NAME "tcp"

char *get_headers(size_t body_length) {
 const char headers[] = "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/html\r\n"
                  "Content-Length: %lu\r\n"
                  "Connection: close\r\n\r\n";

  int needed = snprintf(NULL, 0, headers, body_length); 

  if(needed < 0) return NULL;

  size_t size = (size_t)needed + 1;

  char *headers_buffer = malloc(size);
  if(!headers_buffer) return NULL;

  if(snprintf(headers_buffer, size, headers, body_length) < 0){
    return NULL;
  }

  return headers_buffer;
}

char *read_file(void) {

  FILE *fptr;
  u_long file_size;
  char *file_buffer;

  fptr = fopen("./static/index.html", "r");

  fseek(fptr, 0, SEEK_END);
  file_size = ftell(fptr);
  rewind(fptr);
  file_buffer = (char *)malloc(file_size + 1);
  fread(file_buffer, file_size, 1, fptr);
  fclose(fptr);

  file_buffer[file_size] = '\0';

  printf("%s", file_buffer);

  return file_buffer;
}

int main(void) {
  int sock, new_sock; // socket
  struct sockaddr_storage new_addr;
  socklen_t addr_size;

  struct addrinfo hints, *res, *p;

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

  while (1) { // main accept() loop
    addr_size = sizeof new_addr;
    new_sock = accept(sock, (struct sockaddr *)&new_addr, &addr_size);
    if (new_sock == -1) {
      perror("accept");
      continue;
    }

    if (!fork()) { // this is the child process
      close(sock); // child doesn't need the listener
      long sent_bytes = 0;
      const char* html_file = read_file();
      size_t html_file_size = strlen(html_file);

      const char* headers = get_headers(html_file_size);
      size_t headers_size = strlen(headers);

      size_t full_size = headers_size + html_file_size;
      char *shared_buffer = malloc(full_size + 1);

      strcpy(shared_buffer, headers);
      strncat(shared_buffer, html_file, full_size + 1);

      while (1) {
        sent_bytes += send(new_sock, shared_buffer, full_size, 0);
        if (sent_bytes == -1) {
          perror("send");
          break;
        }
        if (sent_bytes == (full_size))
          break;
      }
      shutdown(new_sock, SHUT_WR);
      close(new_sock);
      exit(0);
    }
    close(new_sock); // parent doesn't need this
  }

  return EXIT_SUCCESS;
}
