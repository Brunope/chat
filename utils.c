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


/**
 * Takes a message and USER, packages it up with the user nick and sends it.
 */
int send_msg(USER *sender, char *msg) {
  char message[MSG_LEN];
  memset(message, 0, sizeof(message));
  serialize(message, sender->nick, msg);
  /* if (DEBUG == ON) { */
  /*   printf("--- sending %lu bytes from %s ---\n", sizeof(PACKET), p.sender_nick); */
  /* } */
  return send_data(sender->sockfd, message);
}

/**
 * Takes a socket file descriptor and a string and sends the string through the
 * socket.
 */
int send_data(int sockfd, char *data) {
  int bytes = send(sockfd, data, MSG_LEN, 0);
  if (bytes != MSG_LEN) {
    fprintf(stderr, "only sent %d bytes\n", bytes);
  }
  return bytes;
}

/**
 * Formats a nick and message string into one string, with the nick separated
 * from the message by a ': '. Puts the result in dst.
 */
void serialize(char *dst, char *nick, char *msg_str) {
  sprintf(dst, "%s: %s", nick, msg_str);
}

