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

#define BACKLOG 5
#define MAX_CLIENTS 10



void *handle_client(void *sockfd);
void send_to_all(PACKET *p);
void add_client(int sockfd);
void rm_client(int sockfd);

// clients is a list of socket file descriptors. The indices in the range
// [0, num_clients-1] contain the file descriptors of every client connecter
// to the server.
int clients[MAX_CLIENTS];

// num_clients is the number of clients currently connected to the server.
int num_clients = 0;

// mutex is a pthread_mutex for protecting our client list across threads
pthread_mutex_t mutex;

// threads contains a thread for every client connected to the server. The id
// of a thread is akin to the index of that thread's fd in the client list.
pthread_t threads[MAX_CLIENTS];

/**
 * Supply 2 extra args.
 * argv[1] - the address of the server to connect to.
 * argv[2] - the port number on which to connect.
 */
int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "--- usage: ./server PORTNO ---\n");
    exit(EXIT_FAILURE);
  }

  pthread_mutex_init(&mutex, NULL);

  struct addrinfo hints, *servinfo;
  socklen_t sin_size;
  char ip4[INET_ADDRSTRLEN];

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // ipv4 only
  hints.ai_socktype = SOCK_STREAM; // stream socket
  hints.ai_flags = AI_PASSIVE; // fill in my ip for me

  if (getaddrinfo(NULL, argv[1], &hints, &servinfo) != 0) {
    fprintf(stderr, "--- getaddrinfo error ---\n");
    return EXIT_FAILURE;
  }
  int sockfd; // our listening socket
  sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
                  servinfo->ai_protocol);
  bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
  freeaddrinfo(servinfo);

  // wait for connections
  listen(sockfd, BACKLOG);
  struct sockaddr_storage client_addr;
  while (1) {
    if (DEBUG == ON) { printf("--- waiting for connections ---\n"); }
    sin_size = sizeof(client_addr);
    int newfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
    if (newfd == -1) {
      perror("accept");
      continue;
    }

    // print the ipv4 address of the client
    if (DEBUG == ON) {
      inet_ntop(client_addr.ss_family,
                &(((struct sockaddr_in *)&client_addr)->sin_addr),
                ip4,
                sizeof(ip4));
      printf("--- got connection from %s ---\n", ip4);
    }
    
    // spawn a client handler, passing the new socket fd
    pthread_create(&threads[num_clients], NULL, handle_client, (void *)&newfd);
  }
}

void *handle_client(void *sock) {
  int sockfd = *(int *)(sock);
  add_client(sockfd); // add the client to list of connected clients

  // recv messages from the client
  while (1) {
    PACKET p;
    memset(&p, 0, sizeof(PACKET));
    int bytes = recv(sockfd, (void *)&p, sizeof(PACKET), 0);
    if (!bytes) {
      if (DEBUG == ON) { printf("--- connection lost from %d ---\n", sockfd); }
      rm_client(sockfd);
      pthread_exit(NULL);
    }
    if (DEBUG == ON) { printf("--- received %d bytes ---\n", bytes); }

    printp(&p); // print the packet
    send_to_all(&p); // send the message to all connected clients
    
  }
  pthread_exit(NULL);
}

void send_to_all(PACKET *p) {
  if (DEBUG == ON) { printf("dispatching to %d clients", num_clients); }
  pthread_mutex_lock(&mutex);
  // loop through all connected clients, and send p to each
  for (int i = 0; i < num_clients; i++) {
    send_packet(clients[i], p);
  }
  pthread_mutex_unlock(&mutex);
}


void add_client(int sockfd) {
  pthread_mutex_lock(&mutex);
  clients[num_clients] = sockfd;
  num_clients++;
  pthread_mutex_unlock(&mutex);
}

void rm_client(int sockfd) {
  close(sockfd);
  pthread_mutex_lock(&mutex);
  for (int i = 0; i < num_clients; i++) {
    if (sockfd == clients[i]) {
      // delete the current client and move the rest down by one
      for (int j = i; j < num_clients - 1; j++) {
        clients[j] = clients[j + 1];
      }
      num_clients--;
      break;
    }
  }
  pthread_mutex_unlock(&mutex);
}
