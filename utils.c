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
 * Takes an IPv4 address and port number and connects. Returns the new socket
 * file descriptor. Prints an error message to stderr and exits if something
 * goes wrong.
 */
int connect_to_server(const char *address, const char *port) {
  // load the address structs with getaddrinfo()
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // IPv4 only
  hints.ai_socktype = SOCK_STREAM; // stream socket
  struct addrinfo *serv;
  getaddrinfo(address, port, &hints, &serv);

  // make a socket and connect
  int sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol);
  connect(sockfd, serv->ai_addr, serv->ai_addrlen);

  freeaddrinfo(serv); // can't forget to free
  return sockfd;
}

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
  /* if (DEBUG == ON) { */
  /*   printf("--- sent %d bytes ---\n", bytes); */
  /* } */
}

/**
 * Formats a nick and message string into one string, with the nick separated
 * from the message by a ': '. Puts the result in dst.
 */
void serialize(char *dst, char *nick, char *msg_str) {
  sprintf(dst, "%s: %s", nick, msg_str);
}

