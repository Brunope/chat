#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#include "utils.h"

#define NUM_THREADS 2

void *listener(void *user);
void printp(PACKET *p);

int main(int argc, char **argv) {

  if (argc != 3) {
    fprintf(stderr, "usage: ./client SERVERADDRESS PORTNO\n");
    exit(EXIT_FAILURE);
  }
  
  // initialize socket connection
  USER self;
  memset(&self, 0, sizeof(USER));
  strcpy(self.nick, "Anonymous"); // default nick
  // argv[1] is hostname, argv[2] is portno
  self.sockfd = connect_to_server(argv[1], argv[2]);
  if (DEBUG == ON)
    printf("--- connected to %s at port %s ---\n", argv[1], argv[2]);
  // start listening to the server for messages
  pthread_t threads[NUM_THREADS];
  pthread_create(&threads[0], NULL, listener, (void *) &self);
  if (DEBUG == ON) { printf("--- started socket listener ---\n"); }

  // start the input loop - send all input data to the server
  char input[MSG_LEN];
  while (fgets(input, MSG_LEN, stdin)) {
    send_msg(&self, input);
  }
}

/**
 * Takes a USER (cast as a void* because this is called from pthread_create),
 * and prints data read from its socket connection. Stops if the recv fails.
 */
void *listener(void *user) {
  USER *u = user;
  
  // read data from the server for new messages
  PACKET p;
  int recvd;
  while (1) {
    memset(&p, 0, sizeof(PACKET));
    recvd = recv(u->sockfd, (void *)&p, sizeof(PACKET), 0);
    if (!recvd) {
      fprintf(stderr, "--- lost listener connection ---\n");
      pthread_exit(NULL);
    } else if (recvd > 0) {
      if (DEBUG == ON) { printf("--- received %d bytes ---\n", recvd); }
      printp(&p); // display the contents of the packet
    }
  }
  pthread_exit(NULL);
}

  
