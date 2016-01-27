#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#include "talker.h"
#include "mlist.h"
#include "../utils/utils.h"

void *listener(void *sockfd);
int connect_to_server(char *address, char *port);

FILE *flog;  // debug file
int sockfd;
int connected;

// return 0 on success, -1 on error
int talker_init(char *address, char *port) {
  new_messages = 0;
  connected = 0;

  // open the log file for debugging
  if (DEBUG == ON) {
    flog = fopen(LOG_FILE, "w");
    if (!flog) {
      fprintf(stderr, "can't open log file\n");
      return -1;
    }
  }

  // open socket connection to server
  sockfd = connect_to_server(address, port);
  if (sockfd == -1) {
    fprintf(stderr, "could not open socket connection\n");
    if (DEBUG == ON) { fclose(flog); }
    return -1;
  } else {
    connected = 1;
    if (DEBUG == ON) {
      fprintf(flog, "connected to %s at port %s\n", address, port);
    }
  }

  // handshake first thing after we connect
  //handshake(sockfd); we don't really need to though since we're not getting
  // a shared key or anything

  // initialize message log
  mlist_init();
  
  // start listening to the server for messages in the background
  pthread_t threads[NUM_THREADS];
  pthread_create(&threads[0], NULL, listener, NULL);
  if (DEBUG == ON) { fprintf(flog, "started socket listener\n"); }

  return 0;
}

// return 1 iff connected
int talker_is_connected() {
  return connected;
}

void talker_handle_msg(char *msg) {
  send(sockfd, msg, BUFFER_LEN, 0);
}

// return number of messages in mlist
int talker_num_msg() {
  return mlist_size();
}

// return ith index message in mlist
char *talker_get_msg(int i) {
  return mlist_get(i);
}

void talker_close() {
  mlist_free();
  close(sockfd);
  if (DEBUG == ON) { fclose(flog); }
}

/**
 * Takes an IPv4 address or hostname and port number and connects. Returns
 *  the new socket file descriptor. Prints an error message to stderr and 
 * exits if something goes wrong.
 */
int connect_to_server(char *address, char *port) {
  // load the address structs with getaddrinfo()
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // IPv4 only
  hints.ai_socktype = SOCK_STREAM; // stream socket

  int err;
  struct addrinfo *serv;
  if (getaddrinfo(address, port, &hints, &serv) == -1) {
    if (DEBUG == ON) {
      err = errno;
      fprintf(flog, "Could not look up address, error %d\n", err);
    }
    freeaddrinfo(serv); // can't forget to free
    return -1;
  }
    

  // open a socket
  int sock = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol);
  if (sock == -1) {
    if (DEBUG == ON) {
      err = errno;
      fprintf(flog, "Could not open socket, error %d\n", err);
    }
    freeaddrinfo(serv);
    return -1;
  }

  // connect
  if (connect(sock, serv->ai_addr, serv->ai_addrlen) == -1) {
    if (DEBUG == ON) {
      err = errno;
      fprintf(flog, "Could not connect to %s at %s, error %d\n",
              address, port, err);
    }
    freeaddrinfo(serv);
    return -1;
  }

  freeaddrinfo(serv);
  return sock;
}

/**
 * Takes a connect()ed sockfd (cast as a void* because this is called from 
 * pthread_create), and reads data from the connection. Each message received
 * is added to a list of messages, (see mlist.c)
 */
void *listener(void *param) {
  char message[MSG_LEN];
  int recvd;
  while (connected) { // read data from the server for new messages
    memset(&message, 0, sizeof(message));
    recvd = recv(sockfd, message, sizeof(message), 0);
    if (!recvd) {
      connected = 0;  // flag disconnect
      if (DEBUG == ON) { fprintf(flog, "lost connection\n"); }
    } else if (recvd > 0) {
      if (DEBUG == ON) {
        fprintf(flog, "received %d bytes\n", recvd);
        fprintf(flog, "%s\n", message);
      }
      // add the message to the list and log it
      if (DEBUG == ON) { mlist_add_log(message, flog); }
      else { mlist_add(message); }     
      new_messages = 1;  // flag the new messages
    }
  }
  pthread_exit(NULL);
}

