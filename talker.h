/**
 * talker is the underlying model in the chat client that actually sends and
 * receives data from the server. After talker_init() is called, talker
 * maintains two threads. One continuously receives data from the server and
 * stores it in a log of inbound messages. The other thread sends data to the
 * server whenever talker_handle_msg() is called.
 */

#define NUM_THREADS 2
#define LOG_FILE "log.txt"

/**
 * Allocate space on the heap for the talker, and start a socket connection
 * with the server at "address" on port "port". If the allocation and the
 * connection suceed, return 0. If either fail, retun -1.
 *
 * talker_init() MUST be called before any other talker methods.
 * You MUST call talker_close() to free space allocated by this function.
 */
int talker_init(char *address, char *port);

/**
 * Return 1 if talker is connected to a server, else return 0.
 */
int talker_is_connected();

/**
 * Send a serialized version of "msg" to the server.
 */
void talker_handle_msg(char *msg);

/**
 * Return the number of messages talker has stored. 
 *
 * At some point (determined
 * by MLIST_CAPACITY in mlist.h), this number won't change anymore. That's
 * because adding to a full mlist deletes the last element to make room for
 * the new one.
 */
int talker_num_msg();

/**
 * Return a heap allocated string representing the "i"th most recently
 * received message from the server, ie talker_get_msg(0) returns the most
 * recent message. If i >= talker_num_msg(), behavior of this function is
 * undefined.
 */
char *talker_get_msg(int i);

/**
 * Free all the space allocated by talker and end all its processes.
 */
void talker_close();

/**
 * "new_messages" is a flag set to true by talker when it receives a new
 * message
 */
int new_messages;
