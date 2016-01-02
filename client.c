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
#include <curses.h>

#include "mlist.h"

#define NUM_THREADS 2
#define DEFAULT_NICK "Anon"
#define LOG_FILE "log.txt"
  
void *listener(void *sockfd);
void display_messages(WINDOW *display);

FILE *flog; // debug file
int new_messages = FALSE; // unread messages flag

/**
 * Supply 2 extra args.
 * argv[1] - the address of the server to connect to.
 * argv[2] - the port number on which to connect.
 */
int main(int argc, char **argv) {

  if (argc != 3) {
    fprintf(stderr, "usage: ./client SERVERADDRESS PORTNO\n");
    exit(EXIT_FAILURE);
  }

  // open log file for debugging
  if (DEBUG == ON) {
    flog = fopen(LOG_FILE, "w");
    if (!flog) {
      fprintf(stderr, "can't open log: %s\n", LOG_FILE);
    }
  }

  // initialize socket connection
  int sockfd = connect_to_server(argv[1], argv[2]);
  if (sockfd < 0) {
    if (DEBUG == ON) { fclose(flog); }
    exit(EXIT_FAILURE);
  } else if (DEBUG == ON) {
    fprintf(flog, "connected to %s at port %s\n", argv[1], argv[2]);
  }

  // handshake first thing after we connect
  //handshake(sockfd); we don't really need to though since we're not getting
  // a shared key or anything
  
  // start listening to the server for messages in the background
  pthread_t threads[NUM_THREADS];
  pthread_create(&threads[0], NULL, listener, (void *) &sockfd);
  if (DEBUG == ON) { fprintf(flog, "started socket listener\n"); }

  // initialize ncurses
  int total_height, total_width; // number of rows, columns of the ncurses window
  initscr();
  getmaxyx(stdscr, total_height, total_width);
  int iw_height = 2;
  int dw_height = total_height - iw_height;
  WINDOW *display_win = newwin(dw_height, total_width, 0, 0);
  WINDOW *input_win = newwin(iw_height, total_width, total_height - iw_height, 0);
  
  // draw the input window
  for (int i = 0; i < total_width; i++) {
    mvwprintw(input_win, 0, i, "-");
  }
  mvwprintw(input_win, 1, 0, ": ");
  wrefresh(input_win);

  // get input data whenever it's there; in the meantime refresh our window
  char input_message[BUFFER_LEN];
  memset(input_message, 0, sizeof(input_message));
  char *current_input = input_message;
  nodelay(input_win, TRUE); // make getch non-blocking
  noecho(); // don't echo keypresses back
  int iw_width; // width of input window
  while (1) {
    // it would be great if getstr could be made not to block, but we have to
    // use getch, so we have to do a lot of stuff ourselves.
    int c = wgetch(input_win);
    if (c == ERR) { // no input
      // write all the messages that fit in the screen if there are new ones
      if (new_messages == TRUE) {
        display_messages(display_win);
        new_messages = FALSE;
        //  move the cursor back to where it was
        wmove(input_win, 1, current_input - input_message + 2);
      }
    } else if (c == 8 || c == 127) { // backspace or delete
      if (current_input > input_message) { // there is something to delete
        wmove(input_win, 1, current_input - input_message + 1);
        wclrtoeol(input_win);
        wrefresh(input_win);
        current_input--;
      }
    } else if (c >= 32 && c <= 126) { // letter, number, or symbol
      // write the input to the message, print the char to the screen, and
      // advance the cursor
      sprintf(current_input, "%c", c);
      iw_width = getmaxx(input_win); // recalculate this since window can resize
      if (current_input - input_message == iw_width - 3) {
        // if we're about to reach the end of the line, set the current char
        // to \n, so the input gets handled at the end of the loop iteration.
        c = 10;
      }
      mvwprintw(input_win, 1, current_input - input_message + 2, "%c", c);
      current_input++;
    }

    // send the message if we've got a newline and the message isn't empty
    if (c == 10 && current_input > input_message) {
      send(sockfd, input_message, BUFFER_LEN, 0);
      //send_data(sockfd, input_message);
      //handle_input(&self, input_message);
      //clean up
      current_input = input_message;
      wmove(input_win, 1, 0);
      wclrtoeol(input_win); // clear the whole line
      wprintw(input_win, ": "); // reprint the prompt
      wrefresh(input_win);
      memset(input_message, 0, sizeof(input_message)); // reset our message
    }
  }

  // clean up all our resources
  close(sockfd);
  if (DEBUG == ON) { fclose(flog); }
}

/**
 * Takes an IPv4 address or hostname and port number and connects. Returns
 *  the new socket file descriptor. Prints an error message to stderr and 
 * exits if something goes wrong.
 */
int connect_to_server(const char *address, const char *port) {
  // load the address structs with getaddrinfo()
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // IPv4 only
  hints.ai_socktype = SOCK_STREAM; // stream socket

  int err;
  struct addrinfo *serv;
  if (getaddrinfo(address, port, &hints, &serv) == -1) {
    err = errno;
    fprintf(stderr, "Could not look up address");
    return err;
  }
    

  // open a socket
  int sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol);
  if (sockfd == -1) {
    err = errno;
    fprintf(stderr, "Could not open socket\n");
    return err;
  }

  // connect
  if (connect(sockfd, serv->ai_addr, serv->ai_addrlen) == -1) {
    err = errno;
    fprintf(stderr, "Could not connect to %s at %s\n", address, port);
    return err;
  }

  freeaddrinfo(serv); // can't forget to free
  return sockfd;
}

/**
 * Takes a connect()ed sockfd (cast as a void* because this is called from 
 * pthread_create), and reads data from the connection. Each message received
 * is added to a list of messages, (see mlist.c)
 */
void *listener(void *sockfd_void) {
  int sockfd = *(int *)sockfd_void;

  mlist_init(); // initialize list of received messages

  char message[MSG_LEN];
  int recvd;
  while (1) { // read data from the server for new messages
    memset(&message, 0, sizeof(message));
    recvd = recv(sockfd, message, sizeof(message), 0);
    if (!recvd) {
      if (DEBUG == ON) { fprintf(stderr, "lost listener connection\n"); }
      pthread_exit(NULL);
    } else if (recvd > 0) {
      if (DEBUG == ON) {
        fprintf(flog, "received %d bytes\n", recvd);
        fprintf(flog, "%s\n", message);
      }
      // add the message to the list and log it
      if (DEBUG == ON) { mlist_add_log(message, flog); }
      else { mlist_add(message); }     
      new_messages = TRUE; // flag the new messages
    }
  }
  mlist_free();
  pthread_exit(NULL);
}

void display_messages(WINDOW *display) {

  int height, width;
  getmaxyx(display, height, width);
  MESSAGE *current = mlist_front();
  // print messages from bottom to top, ordered by time received, ie most recent
  // message is on the bottom
  int i = 1; // our offset from the bottom
  while (i <= height && current != NULL) {
    int chars_left = strlen(current->message);
    // if message is longer than the width of our window
    int trail = chars_left % width;
    mvwprintw(display,
              height - i,
              0,
              "%s\n", current->message + chars_left - trail);
    chars_left -= trail;
    i++;
    while (chars_left > 0) {
      // print the message in chunks of length width or less, with the last
      // chunk on the bottom
      mvwprintw(display,
                height - i,
                0,
                "%s\n", current->message + chars_left - width);
      chars_left -= width;
      i++;
    }
    current = current->next;
  }
  wrefresh(display);
}
  
