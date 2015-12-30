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
void display_messages(WINDOW *display);
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

    // handle the message if we've got a newline and the message isn't empty
    if (c == 10 && current_input > input_message) {
      handle_input(&self, input_message);
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
  char *nick_cmd = "/nick";
  if (strncmp(input, nick_cmd, strlen(nick_cmd)) == 0) {
    // save the old nick real quick
    char old_nick[NICK_LEN];
    strcpy(old_nick, user->nick);
    // set the new one to the token following the command
    char *new_nick = input + strlen(nick_cmd) + 1;
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
  
