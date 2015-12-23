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
#include <curses.h>

//#include "utils.h"
#include "mlist.h"

#define NUM_THREADS 2
#define DEFAULT_NICK "Anon"
#define LOG_FILE "log.txt"
  
void *listener(void *user);
void add_message(MESSAGE *front, char *msg);
void display_messages(WINDOW *display, MESSAGE *front, int max_msgs);

FILE *flog; // debug file
int new_messages = FALSE;

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

  // initialize ncurses
  int row, col; // number of rows, columns of the ncurses window
  initscr();
  getmaxyx(stdscr,row,col);
  WINDOW *display_win = newwin(row - 1, col, 0, 0);
  WINDOW *input_win = newwin(1, col, row - 1, 0);

  // set the max number of messages to be the window size minus 1 line for input
  int max_messages = row - 1;

  // initialize socket connection
  USER self;
  memset(&self, 0, sizeof(USER));
  strcpy(self.nick, DEFAULT_NICK);
  self.sockfd = connect_to_server(argv[1], argv[2]);
  if (DEBUG == ON)
    fprintf(flog, "connected to %s at port %s\n", argv[1], argv[2]);
  
  // start listening to the server for messages in the background
  pthread_t threads[NUM_THREADS];
  pthread_create(&threads[0], NULL, listener, (void *) &self);
  if (DEBUG == ON) { fprintf(flog, "started socket listener\n"); }

  // get input data whenever it's there; in the meantime refresh our window
  char input_message[BUFFER_LEN];
  memset(input_message, 0, sizeof(input_message));
  char *current_input = input_message;
  nodelay(input_win, TRUE); // make getch non-blocking
  wprintw(input_win, ":"); // draw input colon
  wmove(input_win, 0, 2);
  wrefresh(input_win);
  while (1) {
    int c = wgetch(input_win);
    if (c == ERR) { // no input
      // write all the messages that fit in the screen if there are new ones
      if (new_messages == TRUE) {
        display_messages(display_win, mlist_front(), max_messages);
        refresh();
      }
      continue;
    }

    // if we're about to max out the buffer, write the current char, then
    // write a newline, leaving room for the null terminator which we can rely
    // on being there already from memset. Then send the message.
    // room for the null terminator which we can rely on being there already
    // from memset.
    if (current_input - input_message == MSG_LEN - 3) {
      sprintf(current_input, "%c", c);
      current_input++;
      c = 10;
    }

    // write the input to the message
    sprintf(current_input, "%c", c);
    current_input++;

    // send the message after a newline
    if (c == 10) {
      send_msg(&self, input_message);
      current_input = input_message;
      move(row - 1, 1);
      clrtoeol(); // clear the line
      memset(input_message, 0, sizeof(input_message)); // reset our message
    }
  }
}

/**
 * Takes a USER (cast as a void* because this is called from pthread_create),
 * and prints data read from its socket connection. Stops if the recv fails.
 */
void *listener(void *user) {
  USER *u = user;

  mlist_init(); // initialize list of received messages

  char message[MSG_LEN];
  int recvd;
  while (1) { // read data from the server for new messages
    memset(&message, 0, sizeof(message));
    recvd = recv(u->sockfd, (void *)message, sizeof(message), 0);
    if (!recvd) {
      fprintf(stderr, "lost listener connection\n");
      pthread_exit(NULL);
    } else if (recvd > 0) {
      if (DEBUG == ON) { fprintf(flog, "received %d bytes ---\n", recvd); }
      fprintf(stderr, "adding to mlist");
      mlist_add(message); // add the message to the list
      fprintf(stderr, "added to mlist");
      new_messages = TRUE; // flag the new messages
    }
  }
  pthread_exit(NULL);
}

void display_messages(WINDOW *display, MESSAGE *front, int max_msgs) {
  
  return;
}
  
