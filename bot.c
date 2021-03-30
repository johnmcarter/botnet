// Client code for botnet

#include "bot.h"

userInfo_t userInfo;
char username[1000];

int repeated_connect( const char * server, struct addrinfo * rp ) {
    int             sd;
    char            message[256];
    
    do {
        if ( errno = 0, (sd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol )) == -1 ) {
            return -1;
        } else if ( errno = 0, connect( sd, rp->ai_addr, rp->ai_addrlen ) == -1 ) {
            close( sd );
            sleep( 3 );
            write( 1, message, sprintf( message, "\x1b[2;33mConnecting to server %s ...\x1b[0m\n", server ) );
        } else {
            return sd;
        }
    } while ( errno != 0 );
    return -1;
}

int main( int argc, char ** argv ) {
    int             sd;
    int             ignore;
    char            message[256];
    char            ignore2[1000];
    char            welcomeMessage[100];
    //char            username[1000];

    
    struct addrinfo      addrinfo;
    struct addrinfo *    result;
    struct addrinfo *    rp;
    
    addrinfo.ai_flags = 0;
    addrinfo.ai_family = AF_INET;
    addrinfo.ai_socktype = SOCK_STREAM;
    addrinfo.ai_protocol = 0;
    addrinfo.ai_addrlen = 0;
    addrinfo.ai_addr = NULL;
    addrinfo.ai_canonname = NULL;
    addrinfo.ai_next = NULL;
    strcpy(username, argv[3]);
    if ( argc < 4 ) {
        fprintf( stderr, "\x1b[1;31mMust specify server host name, port number and username on command line\x1b[0m\n");
        exit( 1 );
    } else if ( sscanf( argv[2], "%d", &ignore ) == 0 ) {
        fprintf( stderr, "\x1b[1;31mMust specify port number as integer on command line\x1b[0m\n");
        exit( 1 );
    } else if ( sscanf( argv[3], "%s", ignore2 ) == 0 ) {
        fprintf( stderr, "\x1b[1;31mMust specify username on command line\x1b[0m\n");
        exit( 1 );
    } else if ( getaddrinfo( argv[1], argv[2], &addrinfo, &result ) != 0 ) {
        fprintf( stderr, "\x1b[1;31mgetaddrinfo( %s ) reason is %s\x1b[0m\n", argv[1], strerror( errno ));
        exit( 1 );
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
            write(sd, username, strlen(username));
            sprintf(welcomeMessage, "\x1b[1;34mWelcome back, %s!\x1b[0m\n", username);
            write(1, welcomeMessage, strlen(welcomeMessage));
            userInfo.sd = sd;
            strcpy(userInfo.username, username);
            pthread_t commandInputThread;
            pthread_t responseOutputThread;
            
            pthread_create(&commandInputThread, NULL, cit, &sd);
            pthread_create(&responseOutputThread, NULL, rot, &sd);
            
            pthread_join(commandInputThread, NULL);
            pthread_join(responseOutputThread, NULL);
            pthread_cancel(commandInputThread);
            pthread_cancel(responseOutputThread);
            
            return 0;
        }
    }
    return 0;
}

void *cit(void *vargp) {
    int             sd = *((int *) vargp);
    char            string[512];
    char            prompt[] = "Enter a command or message: \n";
    int             len;
    char            user[100];
    while ( write( 1, prompt, sizeof(prompt) ), (len = read( 0, string, sizeof(string) - 1)) > 0 ) {
        string[len] = '\0';
        strcpy(user, userInfo.username);
        strcat(user, " ");
        strcat(user, string);
        write( sd, user, strlen( user ));
        sleep(2);
    }
    close( sd );
    return NULL;
}

void *rot(void *vargp) {
    int   br = 0;
    int   sd = *((int *) vargp);
    char  buffer[512];
    char  output[1024];
    while (1) {
        br = read(sd, buffer, sizeof(buffer) - 1);
        if (br <= 0) {
            sprintf(output, "\x1b[1;31mConnection Terminated.  User %s exiting.\x1b[0m\n", username);
            write( 1, output, strlen(output) );
            close( sd );
            exit(0);
        } else {
            buffer[br] = '\0';
            sprintf( output, "%s", buffer );
            write( 1, output, strlen(output) );
        }
    }
    return NULL;
}