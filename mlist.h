#include "utils.h"

/**
 * mlist is a data structure representing a finite sized list of strings in
 * LIFO order. The most recently added string is returned by mlist_get(0), and
 * the least recently added string is mlist_get(mlist_size() - 1).
 */

#define MLIST_CAPACITY 100

void mlist_init();
void mlist_add(char *data);
void mlist_add_log(char *data, FILE *flog);
int mlist_size();
char *mlist_get(int index);
void mlist_free();

