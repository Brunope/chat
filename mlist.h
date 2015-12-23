#include "utils.h"

#define MLIST_CAPACITY 100

// A MESSAGE is a node in a cyclical linked list. The list as a whole represents
// the ordered list of inbound messages from the server. Initially, new nodes
// (messages) are prepended to the front of the list. Then, when the number of
// nodes reaches MESSAGE_CACHE_SIZE, the list cycles, and adding a MESSAGE
// overwrites the MESSAGE previously in that position. The front of the list
// is always the most recently received MESSAGE.
typedef struct MESSAGE {
  char message[MSG_LEN];
  struct MESSAGE *next;
} MESSAGE;

void mlist_init();
void mlist_add(char *data);
void *mlist_front();
void mlist_free();

