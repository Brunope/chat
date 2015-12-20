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
void send_msg(USER *sender, char *msg) {
  PACKET p;
  memset(&p, 0, sizeof(PACKET));
  strcpy(p.sender_nick, sender->nick);
  strcpy(p.message, msg);
  if (DEBUG == ON) {
    printf("sending '%s' from '%s' (%d) bytes", p.message, p.sender_nick,
           sizeof(PACKET));
  }
  send_packet(sender->sockfd, &p);
}

void send_packet(int sockfd, PACKET *p) {
  int bytes = send(sockfd, (void *)&p, sizeof(PACKET), 0);
  if (DEBUG == ON) {
    printf("sent %s(%d bytes)\n", p->message, bytes);
  }
}

/**
 * Prints the contents of a PACKET
 */
void printp(PACKET *p) {
  printf("%s: %s\n", p->sender_nick, p->message);
}
