#ifndef SERVER_H
#define SERVER_H

#define MESSAGE_LEN 2048
#define CONTENTS_LEN 2046
#define TYPE_LEN 2
#define SAY_MSG_LEN 1790
#define SAYCONT_MSG_LEN 1789
#define SAYCONT_TER 2047
#define TERMINATION 255
#define PIPE_NAME_MAX 256
#define PIPE_PATH_MAX 515

#define CONNECT 0
#define SAY 1
#define SAYCONT 2
#define RECEIVE 3
#define RECVCONT 4
#define PING 5
#define PONG 6
#define DISCONNECT 7


#define POLL_TIMEOUT 1000
#define NFDS 2
#endif
