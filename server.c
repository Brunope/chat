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
#define DEFAULT_NICK "Anon"

typedef struct CLIENT {
  int sockfd;
  char nick[NICK_LEN];
  char address[INET_ADDRSTRLEN]
} CLIENT;

void *handle_client(void *sockfd);
void send_to_all(char *data);
void handle_msg(CLIENT *sender, char *msg);
void handshake(CLIENT *client, int sockfd);
void add_client(CLIENT *client);
void rm_client(int sockfd);

// clients is a list of socket file descriptors. The indices in the range
// [0, num_clients-1] contain the file descriptors of every client connecter
// to the server.
CLIENT clients[MAX_CLIENTS];

// num_clients is the number of clients currently connected to the server.
int num_clients = 0;

// mutex is a pthread_mutex for protecting our client list across threads
pthread_mutex_t mutex;

// threads contains a thread for every client connected to the server. The id
// of a thread is akin to the index of that thread's CLIENT in the list.
pthread_t threads[MAX_CLIENTS];

/**
 * Supply 1 extra arg.
 * argv[1] - the address of the server to connect to.
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

    // handshake first thing after connecting, and store the result in client
    CLIENT client;
    strcpy(client.address, ip4);
    handshake(&client, newfd);
    
    // spawn a client handler, passing the new CLIENT
    pthread_create(&threads[num_clients], NULL, handle_client, (void *)&client);
  }
}

// initialize client with a sockfd and nick, tell client they've connected
void handshake(CLIENT *client, int sockfd) {
  client->sockfd = sockfd;
  strcpy(client->nick, DEFAULT_NICK);

}
void *handle_client(void *client_void) {
  CLIENT *client = client_void;
  int sockfd = client->sockfd;
  add_client(client); // add the client to list of connected clients

  // recv messages from the client
  while (1) {
    char data[BUFFER_LEN];
    memset(data, 0, sizeof(data));
    int bytes = recv(sockfd, (void *)data, sizeof(data), 0);
    if (!bytes) {
      if (DEBUG == ON) { printf("--- connection lost from %d ---\n", sockfd); }
      rm_client(sockfd);
      pthread_exit(NULL);
    }
    if (DEBUG == ON) {
      printf("--- received %d bytes from %s ---\n", bytes, client->ip4);
    }
    printf("%s: %s\n", client->nick, data);
    
    // process whatever the client sent us
    handle_msg(client, data);
  }
  pthread_exit(NULL);
}

void handle_msg(CLIENT *sender, char *msg) {
  char result[MSG_LEN]; // we'll send this out after processing input
  // update client nick if they want to change it
  if (strncmp(msg, "/nick ", 6) == 0) {
    // save the old nick
    char old_nick[NICK_LEN];
    strcpy(old_nick, sender->nick);
    // update with the new one
    strcpy(sender->nick, msg + 6);
    // send an update of the change to all connected clients
    sprintf(result, "%s is now known as %s", old_nick, sender->nick);
    send_to_all(result);
  } else { // otherwise just send the message to all connected clients
    sprintf(result, "%s: %s", sender->nick, msg);
    send_to_all(result);
  }
}

void send_to_all(char *msg) {
  if (DEBUG == ON) { printf("--- dispatching to %d clients ---\n", num_clients); }
  pthread_mutex_lock(&mutex);
  // loop through all connected clients, and send data
  for (int i = 0; i < num_clients; i++) {
    int bytes = send_data(clients[i].sockfd, msg);
    printf("--- sent %d bytes ---\n", bytes);
  }
  pthread_mutex_unlock(&mutex);
}


void add_client(CLIENT *client) {
  pthread_mutex_lock(&mutex);
  clients[num_clients] = *client;
  num_clients++;
  pthread_mutex_unlock(&mutex);
}

void rm_client(int sockfd) {
  close(sockfd);
  pthread_mutex_lock(&mutex);
  for (int i = 0; i < num_clients; i++) {
    if (sockfd == clients[i].sockfd) {
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
