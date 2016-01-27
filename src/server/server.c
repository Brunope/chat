/**
 * server is the backend of the chat program. It listens on some port for
 * incoming client connections, and spawns a new process to handle each
 * connected client. It stores data on each client, such as the client's nick,
 * and address.
 */

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

#include "../utils/utils.h"

#define BACKLOG 5
#define MAX_CLIENTS 10
#define DEFAULT_NICK "Anon"

// CLIENT stores all the data we need for an individual client connected to
// the server.
typedef struct CLIENT {
  int sockfd;
  char nick[NICK_LEN];
  char address[INET_ADDRSTRLEN];
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
CLIENT *clients[MAX_CLIENTS];

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

    // get and print the ipv4 address of the client
    inet_ntop(client_addr.ss_family,
              &(((struct sockaddr_in *)&client_addr)->sin_addr),
              ip4,
              sizeof(ip4));
    printf("--- got connection from %s ---\n", ip4);

    // make a new CLIENT, and copy over its IPv4 address.
    CLIENT *client = malloc(sizeof(CLIENT));
    strcpy(client->address, ip4);

    // handshake to generate a nick and whatever else (shared key coming soon)
    handshake(client, newfd);
    
    // spawn a client handler, passing the new CLIENT
    pthread_create(&threads[num_clients], NULL, handle_client, (void *)client);
  }
}

// initialize client with a sockfd and nick, tell client they've connected
void handshake(CLIENT *client, int sockfd) {
  client->sockfd = sockfd;
  strncpy(client->nick, DEFAULT_NICK, NICK_LEN);
}

// monitor the connection with client_void for incoming data, process it, and
// send the result (usually just the original message plus the sender nick) back
// out to all targeted clients (usually all connected clients).
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
      printf("--- received %d bytes from %s ---\n", bytes, client->address);
    }
    printf("%s: %s\n", client->nick, data);
    
    // process whatever the client sent us
    handle_msg(client, data);
  }
  rm_client(sockfd);
  pthread_exit(NULL);
}

// Check if a string begins with a special command, ie '/nick', and update the
// server's model of the client who sent the command accordingly. If there is
// no special command, simply bundle the message with the sender's nick and send
// it to all connected clients.
void handle_msg(CLIENT *sender, char *msg) {
  char result[MSG_LEN]; // we'll send this out after processing input

  // exit
  if (strncmp(msg, "/exit", 5) == 0) {
    // save the nick
    char nick[NICK_LEN];
    strncpy(nick, sender->nick, NICK_LEN - 1);
    nick[NICK_LEN - 1] = '\0';  // since strncpy doesn't implicitly copy the \0
    
    // delete the client
    rm_client(sender->sockfd);
    // snprintf does make sure there's a \0 though
    snprintf(result, MSG_LEN, "--- %s has disconnected ---", nick);
    send_to_all(result);
    pthread_exit(NULL);
  }
  
  // nick change
  if (strncmp(msg, "/nick ", 6) == 0) {
    // save the old nick
    char old_nick[NICK_LEN];
    strncpy(old_nick, sender->nick, NICK_LEN - 1);
    old_nick[NICK_LEN - 1] = '\0';
    // update with the new one
    strcpy(sender->nick, msg + 6);  // +6 bc we want the string after '/nick '
    // send an update of the change to all connected clients
    snprintf(result,
             MSG_LEN,
             "--- %s is now known as %s ---",
             old_nick,
             sender->nick);
    send_to_all(result);
  }

  // otherwise just send the message to all connected clients
  else {
    snprintf(result, MSG_LEN, "%s: %s", sender->nick, msg);
    send_to_all(result);
  }
}

// Send msg to all currently connected clients.
void send_to_all(char *msg) {
  if (DEBUG == ON) { printf("--- dispatching to %d clients ---\n", num_clients); }
  // potential race condition - we need a lock because at the same time we
  // are probably monitoring for inbound connections, which would cause the
  // addition of a new client to the list. Or an existing client could
  // disconnect.
  pthread_mutex_lock(&mutex);
  // loop through all connected clients, and send data
  for (int i = 0; i < num_clients; i++) {
    int bytes = send(clients[i]->sockfd, msg, MSG_LEN, 0);
    printf("--- sent %d bytes ---\n", bytes);
  }
  pthread_mutex_unlock(&mutex);
}

// Add a client to the list of currently connected clients.
void add_client(CLIENT *client) {
  pthread_mutex_lock(&mutex);
  clients[num_clients] = client;
  num_clients++;
  pthread_mutex_unlock(&mutex);
}

// Remove a client from the list of currently connected clients.
void rm_client(int sockfd) {
  close(sockfd);
  pthread_mutex_lock(&mutex);
  for (int i = 0; i < num_clients; i++) {
    if (sockfd == clients[i]->sockfd) {
      // delete the current client and move the rest left 1 index to cover it.
      free(clients[i]);
      for (int j = i; j < num_clients - 1; j++) {
        clients[j] = clients[j + 1];
      }
      num_clients--;
      break;
    }
  }
  pthread_mutex_unlock(&mutex);
}
