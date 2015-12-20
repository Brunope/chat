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
  printf("connected to %s at port %s\n", argv[1], argv[2]);
  pthread_create(NULL, NULL, listener, (void *) &self);
  printf("started socket listener");
  

  // start the input loop - send all input data to the server
  char input[MSG_LEN];
  while (fgets(input, MSG_LEN, stdin)) {
    send_msg(&self, input);
  }
}

/**
 * Takes a USER (cast as a void* because this is called from pthread_create),
 * and reads data from its socket connection. Stops if the recv fails.
 */
void *listener(void *user) {
  PACKET p;
  int recvd;

  // read loop
  while (1) {
    memset(&p, 0, sizeof(PACKET));
    recvd = recv(((USER *)user)->sockfd, (void *)&p, sizeof(PACKET), 0);
    if (!recvd) {
      fprintf(stderr, "lost listener connection");
      break;
    } else if (recvd > 0) {
      printp(&p);
    }
  }
  pthread_exit(NULL);
}

/**
 * Prints the contents of a PACKET
 */
void printp(PACKET *p) {
  printf("%s: %s\n", p->sender_nick, p->message);
}
  
