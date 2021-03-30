#ifndef server_h
#define server_h

#include    <sys/types.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <errno.h>
#include    <string.h>
#include    <sys/socket.h>
#include    <netdb.h>
#include    <pthread.h>
#include    <stdbool.h>
#include    <sys/types.h>
#include    <fcntl.h>

#define MAXUSERS 20
#define MAXCHATS 10

typedef struct usersInfo {
    int  fd;
    int  userID;
    char username[100];
    bool chats[MAXUSERS];
} usersInfo_t;

typedef struct chatroomInfo_t {
    char name[100];
    char filename[100];
    int  fd;
} chatroomInfo_t;

char *colorArray[MAXUSERS] = {"\033[0;1;32;40m", "\033[0;1;33;40m", "\033[0;1;31;40m", "\033[0;1;37;40m", "\033[0;1;35;40m", "\033[0;1;36;40m",
    "\033[0;1;32;44m", "\033[0;1;33;44m", "\033[0;1;31;44m", "\033[0;1;37;44m", "\033[0;1;35;44m", "\033[0;1;36;44m", "\033[0;1;36;45m", "\033[0;1;32;47m", "\033[0;1;33;47m", "\033[0;1;31;47m", "\033[0;1;37;46m", "\033[0;1;35;47m", "\033[0;1;36;47m", "\033[0;1;30;47m"
};

char    *parseInput(char *arg, char *clientMSG, usersInfo_t *user);
void    *sessionAcceptor(void *vargp);
void    *clientService(void *vargp);

#endif