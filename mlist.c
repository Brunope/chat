#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mlist.h"

MESSAGE *front; // most recent message added to the list
MESSAGE *back; // least recent message added to the list

/**
 * Allocate space on the heap for all the nodes that fit.
 */
void mlist_init() {
  back = malloc(sizeof(MESSAGE));
  back->next = NULL;
  front = back;
  MESSAGE *new_msg;
  for (int i = 0; i < MLIST_CAPACITY - 1; i++) {
    new_msg = malloc(sizeof(MESSAGE));
    // prepend the node
    new_msg->next = front;
    front = new_msg;
  }
}

void mlist_add(char *data) {
  if (front == NULL) { // front is never null after initialization
    fprintf(stderr, "must call mlist_init() before mlist_add()");
    return;
  }

  // land on the second-to-last node
  MESSAGE *curr = front;
  // curr->next will only ever be NULL if MLIST_CAPACITY is 1, which is a
  // ridiculous setting. But for the sake of writing robust code, we'll check.
  while (curr->next != NULL && curr->next->next != NULL) {
    curr = curr->next;
  }

  // move the back to the front with all the data
  memset(back, 0, sizeof(MESSAGE));
  strcpy(back->message, data);
  back->next = front;
  front = back;

  // set the penultimate node to the end node
  curr->next = NULL;
  back = curr;
}

/**
 * Overwrite the contents of the last node in the list and move it to the front.
 * Make the second-to-last node the last node (point its next to NULL).
 */
void mlist_add_log(char *data, FILE *flog) {
  fprintf(flog, "adding to mlist\n");
  mlist_add(data);
  fprintf(flog, "message log:\n\t%s\t%s", front->message, front->next->message);
}

void *mlist_front() {
  return front;
}

// Free the whole list
void mlist_free() {
  return;
}
