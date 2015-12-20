#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "utils.h"

#define BACKLOG 10

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "usage: ./server PORTNO\n");
    exit(EXIT_FAILURE);
  }

  int sockfd, new_fd;

  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage client_addr;
  socklen_t sin_size;
  char ip4[INET_ADDRSTRLEN];
  int yes = 1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // ipv4 only
  hints.ai_socktype = SOCK_STREAM; // stream socket
  hints.ai_flags = AI_PASSIVE; // fill in my ip for me

  if (getaddrinfo(NULL, argv[1], &hints, &servinfo) != 0) {
    fprintf(stderr, "getaddrinfo error");
    return EXIT_FAILURE;
  }

  sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
                  servinfo->ai_protocol);
  bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
  freeaddrinfo(servinfo);

  listen(sockfd, BACKLOG);

  printf("waiting for connections...\n");

  // accept() loop
  while (1) {
    sin_size = sizeof(client_addr);
    new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(client_addr.ss_family,
              &(((struct sockaddr_in *)&client_addr)->sin_addr),
              //get_in_addr((struct sockaddr *)&client_addr),
              ip4,
              sizeof(ip4));
    printf("got connection from %s\n", ip4);
    // recv()
    while (1) {
      PACKET *p;
      memset(p, 0, sizeof(PACKET));
      char recv_msg[MSG_LEN];
      int bytes = recv(new_fd, (void *)p, MSG_LEN, 0);
      if (!bytes) {
        fprintf(stderr, "connection lost");
        break;
      }
      printf("received %d bytes\n", bytes);
      printf("%s: %s", p->sender_nick, p->message);
      // send the message to all connected clients
      send_packet(new_fd, p);
      //printf("received %d bytes\nmessage: %s\n", bytes, recv_msg);
    }
  }
}
              
