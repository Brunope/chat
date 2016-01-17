/**
 * mlist is a data structure representing a finite sized list of strings in
 * LIFO order. The most recently added string is returned by mlist_get(0), and
 * the least recently added string is mlist_get(mlist_size() - 1).
 */

#define MLIST_CAPACITY 1000

/**
 * Allocate memory for the mlist. If it fails, return -1, otherwise return
 * 0.
 */
int mlist_init();

/**
 * Prepend a message to the list. data is copied over into the mlist, so it
 * doesn't need to be malloc'd. If mlist is full (ie size == capacity),
 * overwrite the contents of the least recently received message with the new
 * message.
 */
void mlist_add(char *data);

/**
 * mlist_add(), then write the contents of the mlist to flog.
 */
void mlist_add_log(char *data, FILE *flog);

/**
 * Return the message stored at the abstracted index. For example,
 * mlist_get(0) returns the first and most recently added message in the list.
 */
char *mlist_get(size_t index);

/**
 * Return the number of messages in the mlist. Note that when mlist reaches
 * capacity, size won't change anymore.
 */
size_t mlist_size();

/**
 * Free all the memory allocated by mlist.
 */
void mlist_free();
