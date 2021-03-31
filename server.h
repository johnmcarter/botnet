#ifndef server_h
#define server_h

#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <errno.h>
#include    <string.h>
#include    <sys/socket.h>
#include    <sys/types.h>
#include    <sys/types.h>
#include    <netdb.h>
#include    <pthread.h>
#include    <stdbool.h>
#include    <fcntl.h>

#define MAXBOTS 20

typedef struct bot {
    int  fd;
    int  botID;
    char username[100];
} bot_t;

void    *sessionAcceptor(void *vargp);
void    *botService(void *vargp);

#endif