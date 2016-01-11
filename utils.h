#ifndef UTILS_H
#define UTILS_H

#define BUFFER_LEN 234
#define NICK_LEN 20
#define MSG_LEN BUFFER_LEN + NICK_LEN + 2 // length of serialized messages

#define DEBUG 1
#define OFF 0
#define ON 1

typedef struct USER {
  int sockfd; // user's socket descriptor
  char nick[NICK_LEN]; // user's name
} USER;

int connect_to_server(const char *address, const char *port);
int send_msg(USER *sender, char *msg);
int send_data(int sockfd, char *data);
void serialize(char *dst, char *nick, char *msg_str);

#endif
