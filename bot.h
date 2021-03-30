#ifndef bot_h
#define bot_h

#include    <sys/types.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <errno.h>
#include    <string.h>
#include    <sys/socket.h>
#include    <netdb.h>
#include    <pthread.h>

void *cit(void *vargp);
void *rot(void *vargp);

typedef struct user {
    int sd;
    char username[100];
} userInfo_t;

#endif