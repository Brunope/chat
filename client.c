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

// #include "utils.h" mlist uses utils so we don't need to include it here
#include "mlist.h"

#define NUM_THREADS 2
#define DEFAULT_NICK "Anon"
#define LOG_FILE "log.txt"
  
void *listener(void *user);
void display_messages(WINDOW *display, int max_msgs);
void handle_input(USER *user, char *input);

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

  // initialize ncurses
  int row, col; // number of rows, columns of the ncurses window
  initscr();
  getmaxyx(stdscr,row,col);
  WINDOW *display_win = newwin(row - 2, col, 0, 0);
  WINDOW *input_win = newwin(2, col, row - 2, 0);

  // set the max number of messages to height of display window
  int max_messages = getmaxy(display_win);
  
  // draw the input window
  for (int i = 0; i < col; i++) {
    mvwprintw(input_win, 0, i, "-");
  }
  mvwprintw(input_win, 1, 0, ": ");
  wrefresh(input_win);

  // get input data whenever it's there; in the meantime refresh our window
  char input_message[BUFFER_LEN];
  memset(input_message, 0, sizeof(input_message));
  char *current_input = input_message;
  nodelay(input_win, TRUE); // make getch non-blocking
  while (1) {
    // it would be great if getstr could be made not to block, but we have to
    // use getch, so we have to do a lot of stuff ourselves.
    int c = wgetch(input_win);
    if (c == ERR) { // no input
      // write all the messages that fit in the screen if there are new ones
      if (new_messages == TRUE) {
        display_messages(display_win, max_messages);
        new_messages = FALSE;
        //  move the cursor back to where it was
        wmove(input_win, 1, current_input - input_message + 2);
      }
      continue;
    }

    if (c == 8 || c == 127) { // backspace or delete
      if (current_input > input_message) { // there is something to delete
        wmove(input_win, 1, current_input - input_message + 1);
        wclrtoeol(input_win);
        wrefresh(input_win);
        current_input--;
      }
      continue;
    }

    // if we're about to reach the ond of the line, write the current char, then
    // write a newline, leaving room for the null terminator which we can rely
    // on being there already from memset. Then send the message.
    // room for the null terminator which we can rely on being there already
    // from memset.
    if (current_input - input_message == col - 3) {
      sprintf(current_input, "%c", c);
      current_input++;
      c = 10;
    }

    // handle the message if we've got a newline
    if (c == 10) {
      handle_input(&self, input_message);

      //clean up
      current_input = input_message;
      wmove(input_win, 1, 0);
      wclrtoeol(input_win); // clear the line
      wprintw(input_win, ": ");
      wrefresh(input_win);
      memset(input_message, 0, sizeof(input_message)); // reset our message
      continue;
    }

    // otherwise just write the input to the message
    sprintf(current_input, "%c", c);
    current_input++;
  }

  // clean up all our resources
  mlist_free();
  close(self.sockfd);
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
    recvd = recv(u->sockfd, message, sizeof(message), 0);
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
  pthread_exit(NULL);
}

void handle_input(USER *user, char *input) {
  // check for /nick
  char *token = strtok(input, " ");
  if (strcmp(token, "/nick") == 0) {
    // save the old nick real quick
    char old_nick[NICK_LEN];
    strcpy(old_nick, user->nick);
    // set the new one to the token following the command
    char *new_nick = strtok(NULL, " ");
    strcpy(user->nick, new_nick);
    // write an event and send it
    char event[BUFFER_LEN];
    sprintf(event, "--- %s is now known as %s ---", old_nick, user->nick);
    send_data(user->sockfd, event);
  }
  // send the message otherwise
  else {
    send_msg(user, input);
  }
}

void display_messages(WINDOW *display, int max_msgs) {

  int i = 1;
  MESSAGE *current = mlist_front();
  while (i <= max_msgs && current != NULL) {
    mvwprintw(display, max_msgs - i, 0, "%s\n", current->message);
    i++;
    current = current->next;
  }
  wrefresh(display);
}
  
