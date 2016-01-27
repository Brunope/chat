/**
 * client is the view and controller of the chat client program. It starts the
 * talker, then loads an ncurses interface. Whenever the user gives input,
 * client sends the input to the talker. In the meantime (whenever the user
 * isn't actively entering input), client displays any new messages talker got
 * from the server.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>

#include "talker.h"
#include "../utils/utils.h"

void display_messages(WINDOW *display);

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

  // initialize the talker program - connects to server and starts listening
  if (talker_init(argv[1], argv[2]) == -1) {
    fprintf(stderr, "error: could not initialize talker\n");
    exit(EXIT_FAILURE);
  }

  // initialize ncurses
  int total_height, total_width; // number of rows, columns of the ncurses window
  initscr();
  getmaxyx(stdscr, total_height, total_width);
  int iw_height = 2;
  int iw_width = total_width;
  int dw_height = total_height - iw_height;
  WINDOW *display_win = newwin(dw_height, total_width, 0, 0);
  WINDOW *input_win = newwin(iw_height, total_width, total_height - iw_height, 0);
  
  // draw the input window
  char connect_msg[MSG_LEN];
  snprintf(connect_msg, MSG_LEN, "connected to %s on port %s-", argv[1],argv[2]);
  for (int i = 0; i < iw_width - strlen(connect_msg); i++) {
    mvwprintw(input_win, 0, i, "-");
  }
  wprintw(input_win, "%s", connect_msg);
  mvwprintw(input_win, 1, 0, ": ");
  wrefresh(input_win);

  nodelay(input_win, TRUE);  // make getch non-blocking
  noecho();                  // don't echo keypresses back
  keypad(input_win, TRUE);   // enable function and arrow keys so we don't echo

  // get input data whenever it's there; in the meantime refresh our window
  char input_message[BUFFER_LEN];
  memset(input_message, 0, sizeof(input_message));
  char *current_input = input_message;
  while (talker_is_connected()) {
    iw_width = getmaxx(input_win);  // recalculate since window can resize
    // it would be great if getstr could be made not to block, but we have to
    // use getch and so we have to do a lot of stuff ourselves.
    int c = wgetch(input_win);
    if (c == ERR) {                 // no input
      // write all the messages that fit in the screen if there are new ones
      if (new_messages == TRUE) {
        display_messages(display_win);
        new_messages = FALSE;
        // move the cursor back to where it was
        wmove(input_win, 1, current_input - input_message + 2);
      }
    } else if (c == 8 || c == 127) {        // backspace or delete
      if (current_input > input_message) {  // there is something to delete
        wmove(input_win, 1, current_input - input_message + 1);
        wclrtoeol(input_win);  // position the cursor and delete the char
        wrefresh(input_win);
        current_input--;
      }
    } else if (c >= 32 && c <= 126) {       // letter, number, or symbol
      // write the input to the message, print the char to the screen, and
      // advance the cursor
      sprintf(current_input, "%c", c);
      if (current_input - input_message == iw_width - 3) {
        // if we're about to reach the end of the line, set the current char
        // to \n, so the input gets handled at the end of the loop iteration.
        c = 10;
      }
      mvwprintw(input_win, 1, current_input - input_message + 2, "%c", c);
      current_input++;
    }

    // process the message if we've got a newline and the message isn't empty
    if (c == 10 && current_input > input_message) {
      talker_handle_msg(input_message);
      //clean up
      current_input = input_message;
      wmove(input_win, 1, 2);
      wclrtoeol(input_win);  // clear everything on the line after the prompt
      wrefresh(input_win);
      memset(input_message, 0, sizeof(input_message));
    }
  }

  // clean up all our resources
  endwin();
  talker_close();

  return EXIT_SUCCESS;
}
  
void display_messages(WINDOW *display) {

  int height, width;
  getmaxyx(display, height, width);
  int total_messages = talker_num_msg();
  // print messages from bottom to top, ordered by time received, ie most recent
  // message is on the bottom
  int offset = 1;  // our offset from the bottom of the window
  int counter = 0;
  while (offset <= height && counter < total_messages) {
    char *msg = talker_get_msg(counter);
    int chars_left = strlen(msg);
    // if message is longer than the width of our window
    int trail = chars_left % width;
    mvwprintw(display,
              height - offset,
              0,
              "%s\n", msg + chars_left - trail);
    chars_left -= trail;
    offset++;
    counter++;
    while (chars_left > 0) {
      // print the message in chunks of length width or less, with the last
      // chunk on the bottom
      mvwprintw(display,
                height - offset,
                0,
                "%s\n", msg + chars_left - width);
      chars_left -= width;
      offset++;
    }
  }
  wrefresh(display);
}
  
