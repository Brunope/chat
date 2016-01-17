#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "mlist.h"
#include "utils.h"

/**
 * mlist uses a circular array of static size. It stores pointers to the "front"
 * and "back" of the list, which move around as messages are added.
 */

int mod(int a, int b);

char **mlist = NULL;  // the actual front of the array
size_t findex, bindex;   // indices of the abstracted front and back of the list
size_t size;             // number of elements

int mlist_init() {
  // allocate space for the pointers
  mlist = (char **) malloc(MLIST_CAPACITY * sizeof(mlist));
  if (mlist == NULL) {
    fprintf(stderr, "couldn't allocate memory for mlist\n");
    return -1;
  }
  // allocate space for the messages themselves
  for (int i = 0; i < MLIST_CAPACITY; i++) {
    mlist[i] = (char *) malloc(MSG_LEN);
    if (mlist[i] == NULL) {
      fprintf(stderr, "couldn't allocate memory for mlist\n");
      return -1;
    }
  }
  findex = MLIST_CAPACITY - 1;
  bindex = findex;
  size = 0;
  return 1;
}

char *mlist_get(size_t index) {
  if (index >= size) {
    fprintf(stderr, "mlist_get index out of range\n");
    return NULL;
  }
  // to turn the abstracted index into the real one, treat it like an offset
  // from the front of the array, and then mod it by the size to wrap around
  return mlist[mod(findex + index, MLIST_CAPACITY)];
}

void mlist_add(char *data) {
  if (mlist == NULL) {
    fprintf(stderr, "must call mlist_init() before mlist_add()\n");
    return;
  }
  if (size == MLIST_CAPACITY) {
    // clobber the least recently added value
    strncpy(mlist[bindex], data, MSG_LEN);
    // move both indices back 1, cycling around if necessary
    findex = mod(findex - 1, MLIST_CAPACITY);
    bindex = mod(bindex - 1, MLIST_CAPACITY);
  } else {
    findex--;
    strcpy(mlist[findex], data);
    size++;
  }
}

size_t mlist_size() {
  return size;
}

void mlist_add_log(char *data, FILE *flog) {
  fprintf(flog, "adding to mlist\n");
  mlist_add(data);
  fprintf(flog, "mlist contents:\n");
  for (size_t i = 0; i < size; i++) {
    fprintf(flog, "\t%s\n", mlist_get(i));
  }
}

void mlist_free() {
  for (int i = 0; i < MLIST_CAPACITY; i++) {
    free(mlist[i]);
  }
  free(mlist);
  mlist = NULL;
}
