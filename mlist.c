#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mlist.h"

/**
 * mlist uses a circular array of static size. It stores pointers to the "front"
 * and "back" of the list, which move around as messages are added.
 */

int mod(int a, int b);

char **mlist = NULL;  // the actual front of the array
int findex, bindex;  // indices of the abstracted front and back of the list
int size;

/**
 * Allocate space on the heap for all the nodes that fit.
 */
void mlist_init() {
  // allocate space for the pointers
  mlist = (char **) malloc(MLIST_CAPACITY * sizeof(mlist));
  if (mlist == NULL) {
    fprintf(stderr, "couldn't allocate memory for mlist\n");
    exit(EXIT_FAILURE);
  }
  // allocate space for the messages themselves
  for (int i = 0; i < MLIST_CAPACITY; i++) {
    mlist[i] = (char *) malloc(MSG_LEN);
    if (mlist[i] == NULL) {
      fprintf(stderr, "couldn't allocate memory for mlist\n");
    }
  }
  findex = MLIST_CAPACITY - 1;
  bindex = findex;
  size = 0;
}

/**
 * Return the message stored at the abstracted "index." For example,
 * mlist_get(0) returns the "first" and most recently added message in the list.
 */
char *mlist_get(int index) {
  if (index < 0 || index >= size) {
    fprintf(stderr, "mlist_get index out of range\n");
    return NULL;
  }
  // to turn the abstracted index into the real one, treat it like an offset
  // from the front of the array, and then mod it by the size to wrap around
  return mlist[mod(findex + index, MLIST_CAPACITY)];
}

/**
 * Prepend a message to the list. If mlist is full (ie size == capacity),
 * delete the least recently added message and use its memory for the new
 * message.
 */
void mlist_add(char *data) {
  if (mlist == NULL) {
    fprintf(stderr, "must call mlist_init() before mlist_add()\n");
    return;
  }
  if (size == MLIST_CAPACITY) {
    // clobber the least recently added value
    strcpy(mlist[bindex], data);
    // move both indices back 1, cycling around if necessary
    findex = mod(findex - 1, MLIST_CAPACITY);
    bindex = mod(bindex - 1, MLIST_CAPACITY);
  } else {
    findex--;
    strcpy(mlist[findex], data);
    size++;
  }
}

/**
 * Return the number of messages in the mlist.
 */
int mlist_size() {
  return size;
}

/**
 * Add a message and write the entire contents of the mlist to a file.
 */
void mlist_add_log(char *data, FILE *flog) {
  fprintf(flog, "adding to mlist\n");
  mlist_add(data);
  fprintf(flog, "mlist contents:\n");
  for (int i = 0; i < size; i++) {
    fprintf(flog, "\t%s\n", mlist_get(i));
  }
}

/**
 * Free the entire mlist and reset its size to 0.
 */
void mlist_free() {
  for (int i = 0; i < MLIST_CAPACITY; i++) {
    free(mlist[i]);
  }
  free(mlist);
  mlist = NULL;
  size = 0;
}
