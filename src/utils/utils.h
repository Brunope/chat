#ifndef UTILS_H
#define UTILS_H

#define BUFFER_LEN 234
#define NICK_LEN 20
#define MSG_LEN BUFFER_LEN + NICK_LEN + 2  // message formatting buffer

#define OFF 0
#define ON 1
#define DEBUG ON

int mod(int a, int b);

#endif
