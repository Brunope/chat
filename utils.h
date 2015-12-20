#define MSG_LEN 256
#define NICK_LEN 20
#define DEBUG 1
#define OFF 0
#define ON 1

typedef struct USER {
  int sockfd; // user's socket descriptor
  char nick[NICK_LEN]; // user's name
} USER;

typedef struct PACKET {
  char sender_nick[NICK_LEN];
  char message[MSG_LEN];
} PACKET;

int connect_to_server(const char *address, const char *port);
void send_msg(USER *sender, char *msg);
void send_packet(int sockfd, PACKET *p);
void printp(PACKET *p);
