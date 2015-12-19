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

#include "utils.h"

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

  // start the input loop - send all input data to the server
  char input[MSG_LEN];
  while (fgets(input, MSG_LEN, stdin)) {
    send_msg(&self, input);
  }
}
