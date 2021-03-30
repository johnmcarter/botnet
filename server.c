#include "server.h"

pthread_mutex_t chatMutex = PTHREAD_MUTEX_INITIALIZER;

static  usersInfo_t users[MAXUSERS];
static  chatroomInfo_t chats[MAXCHATS];

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
        fprintf( stderr, "\x1b[1;31mMust specify number on command line\x1b[0m\n");
        exit( 1 );
    } else if ( sscanf( argv[1], "%d", &ignore ) == 0 ) {
        fprintf( stderr, "\x1b[1;31mMust specify port number as integer on command line\x1b[0m\n");
        exit( 1 );
    } else if ( (sd = claim_port( argv[1] )) == -1 ) {
        write( 1, message, sprintf( message,  "\x1b[1;31mCould not bind to port %s reason %s\x1b[0m\n", argv[1], strerror( errno ) ) );
        return 1;
    } else {
        pthread_t sessionAcceptorThread;
        pthread_create(&sessionAcceptorThread, NULL, sessionAcceptor, &sd);
        pthread_join(sessionAcceptorThread, NULL);
        printf("No longer accepting incoming connections\n");
        close( sd );
        return 0;
    }
}

void *sessionAcceptor(void *vargp) {
    char                    errorMessage[100];
    char                    username[100];
    char                    returnmessage[1000];
    int                     fd;
    int                     br;
    int                     sd = *((int *) vargp);
    int                     i = 0;
    int                     j = 0;
    socklen_t               ic;
    struct sockaddr_in      senderAddr;
    pthread_t               clientServiceThread;
    
    for (i = 0; i < MAXUSERS; i++) {
        users[i].fd = -1;
        users[i].username[0] = '\0';
        for (j = 0; j < MAXCHATS; j++) {
            users[i].chats[j] = false;
        }
    }

    for (i = 0; i < MAXCHATS; i++) {
        chats[i].name[0] = '\0';
        chats[i].filename[0] = '\0';
        chats[i].fd = -1;
    }
    //initialize default chatroom
    strcpy(chats[0].name, "defaultchat");
    strcpy(chats[0].filename, "defaultchat.txt");
    //opening default chat file
    if ((chats[0].fd = open(chats[0].filename, O_RDWR|O_CREAT, 0644)) == -1) {
        fprintf(stderr, "\x1b[1;31mCould not open output file %s. Reason: %s\x1b[0m\n", chats[0].filename, strerror(errno));
        exit(1);
    }
    
    while ((fd = accept(sd, (struct sockaddr *)&senderAddr, &ic )) != -1) {
        i = 0;
        while ((users[i].fd != -1) && (i <= MAXUSERS)) {
            i++;
            printf("i = %d\n", i);
        }

        if (i >= MAXUSERS) {
            //only 20 users
            sprintf(errorMessage, "ERROR: %d users already in the chatroom. Cannot create another session!\n", MAXUSERS);
            write(fd, errorMessage, strlen(errorMessage));
            close(fd);
        } else {
            //less than 20 users, so check for duplicate usernames
            br = read(fd, username, sizeof(username) - 1);
            username[br] = '\0';
            printf("username:  %s fd: %d\n", username, fd);
            //check the username
            i = 0;
            while ((users[i].username[0] != '\0') && (strcmp(username, users[i].username) != 0)) {
                i++;
            }
            if (strcmp(username, users[i].username) == 0) {
                sprintf(returnmessage, "\x1b[1;31mUsername \x1b[1;34m%s\x1b[0m \x1b[1;31mis already in chat! Pick a new one...\x1b[0m\n", username);
                write(fd, returnmessage, strlen(returnmessage));
                close(fd);
                //pthread_exit(0);
            } else {
                strcpy(users[i].username, username);
                users[i].fd = fd;
                users[i].userID = i;
                users[i].chats[0] = true;
                pthread_create(&clientServiceThread, NULL, clientService, &(users[i]));
            }
        }
    }
    return NULL;
}

void *clientService(void *vargp) {
    char                clientMSG[1000];
    char                request[2048];
    char                *savedPointer;
    char                *username;
    char                *arg;
    char                returnmessage[1000];
    char                buffer[3000];
    usersInfo_t *user = (usersInfo_t *)vargp;
    int                 i = 0;
    int                 br = 0;
    int                 j = 0;
    int                 rc = 0;

    //read from default chat file and print to user
    pthread_mutex_lock(&chatMutex);
    rc = lseek(chats[0].fd, 0, SEEK_SET);
    while ((br = read(chats[0].fd, buffer, sizeof(buffer))) > 0) {
        if (br < 0) {
            fprintf(stderr, "\x1b[1;31mError reading from file %s. Reason: %s\x1b[0m\n", chats[0].filename, strerror(errno));
            pthread_mutex_unlock(&chatMutex);
            exit(1);
        }
        write(user->fd, buffer, br);
    }
    pthread_mutex_unlock(&chatMutex);
    
    while ((br = read(user->fd, request, sizeof(request))) > 0) {
        request[br] = '\0';
        username = strtok_r(request, " \n", &savedPointer);
        if (username != NULL) {
            arg = strtok_r(NULL, "\n", &savedPointer);
        }
        
        if (arg != NULL) {
            if (strncmp(arg, "@", 1) == 0) {
                parseInput(arg, clientMSG, user);
                write(user->fd, clientMSG, strlen(clientMSG));
            } else {
                //write to files for the chats this user is in
                for (i = 0; i < MAXCHATS; i++) {
                    sprintf(returnmessage, "%s: %s%s: %s\x1b[0m\n", chats[i].name, colorArray[user->userID], username, arg);
                    if (users[user->userID].chats[i] == true) {
                        printf("\x1b[1;32mWriting to file %s\x1b[0m\n", chats[i].filename);
                        rc = write((chats[i].fd), returnmessage, strlen(returnmessage));
                        if (rc < 0) {
                            fprintf(stderr, "\x1b[1;31mError writing to file %s. Reason: %s\x1b[0m\n", chats[i].filename, strerror(errno));
                            exit(1);
                        }
                        
                    }
                    //write to users in the same chats as the user who sent message
                    for (j = 0; j < MAXUSERS; j++) {
                        if ((users[j].chats[i] == true) && (users[user->userID].chats[i] == true)) {
                            printf("\x1b[1;32mWriting to client %s\x1b[0m\n", users[j].username);
                            write((users[j].fd), returnmessage, strlen(returnmessage) + 1);
                        }
                    }
                }
            }
        }
    }
    //printf("Closing user %s errno %s br %d\n", username, strerror(errno), br);
    close(chats[0].fd);
    return NULL;
}

char *parseInput(char *argument, char *clientMSG, usersInfo_t *user) {
    int   i = 0;
    int   j = 0;
    int   file = 0;
    int   chatcount = 0;
    int   br = 0;
    int   bw = 0;
    int   rc = 0;
    char  *name;
    char  *command;
    char  *arg;
    char  *savedPointer;
    char  filename[100];
    char  buffer[3000];
    char  clientMSG2[3000];
    
    command = strtok_r(argument, " \n", &savedPointer);
    
    if (command != NULL) {
        arg = strtok_r(NULL, " \n", &savedPointer);
    }
    
    if (command == NULL) {
        strcpy(clientMSG, "ERROR: You didn't enter a command!\n");
        
    } else if ((strcmp(command, "@create") == 0)) {
        //Make a new chatroom.  Make sure no others have this name
        if (arg == NULL) {
            strcpy(clientMSG, "ERROR: No chatroom name specified!\n");
        } else if (chatcount == 10) {
            strcpy(clientMSG, "ERROR: 10 chatrooms are active. Can't create any more!\n");
        } else if (strlen(arg) > 100) {
            strcpy(clientMSG, "ERROR: Chatroom name must be at most 100 characters!\n");
        } else {
            //Stepping through names to see if we can make a new account
            while (pthread_mutex_trylock( &chatMutex ) == EBUSY) {
                sprintf(clientMSG, "Waiting to open chatroom %s...\n", arg);
                write(user->fd, clientMSG, strlen(clientMSG) + 1);
                sleep(2);
            }
            while ((strcmp(arg, chats[i].name) != 0) && (strlen(chats[i].name) != 0) && (i < MAXCHATS)) {
                i++;
            }
            //sleep(10);
            if (i == MAXCHATS) {
                strcpy(clientMSG, "ERROR: Max number of chatrooms!\n");
            } else if (strlen(chats[i].name) == 0) {
                strcpy(chats[i].name, arg);
                sprintf(clientMSG, "\x1b[1;32mChatroom %s has been created.  This is chatroom number %d.\x1b[0m\n", chats[i].name, i + 1);
                
                //get name for file and create file. increment counter
                sprintf(filename, "%s.txt", chats[i].name);
                if ((file = open(filename, O_RDWR|O_CREAT, 0644)) == -1) {
                    fprintf(stderr, "\x1b[1;31mCould not open output file %s. Reason: %s\x1b[0m\n",chats[i].name, strerror(errno));
                    exit(1);
                }
                strcpy(chats[i].filename, filename);
                printf("Opening file %s\n", chats[i].filename);
                chats[i].fd = file;
                chatcount++;
                 
            } else {
                strcpy(clientMSG, "ERROR: This chatroom already exists!\n");
            }
            pthread_mutex_unlock( &chatMutex );
        }
    } else if (strcmp(command, "@enter") == 0) {
        //join a chatroom that has already been made
        if (arg == NULL) {
            strcpy(clientMSG, "ERROR: You didn't enter an argument!\n");
        } else {
            name = arg;
            int i = 0;
            while ((strcmp(name, chats[i].name) != 0) && (i < MAXCHATS)) {
                i++;
            }
            if (i == MAXCHATS) {
                //we didn't find chatroom
                strcpy(clientMSG, "ERROR: We can't find this chatroom!\n");
            } else {
                //enter chatroom
                sprintf(clientMSG, "You have entered chatroom %s!\n", chats[i].name);
                printf("Opening file %s\n", chats[i].filename);
                rc = open(chats[i].filename, O_RDWR, 0644);
                if (rc < 0) {
                    fprintf(stderr, "\x1b[1;31mError opening file %s. Reason: %s\x1b[0m\n", chats[i].filename, strerror(errno));
                    exit(1);
                }
                pthread_mutex_lock(&chatMutex);
                lseek(chats[i].fd, 0, SEEK_SET);
                while ((br = read(chats[i].fd, buffer, sizeof(buffer))) > 0) {
                    if (br < 0) {
                        fprintf(stderr, "\x1b[1;31mError reading from file %s. Reason: %s\x1b[0m\n", chats[i].filename, strerror(errno));
                        pthread_mutex_unlock(&chatMutex);
                        exit(1);
                    }
                    if ((bw = write(user->fd, buffer, br)) < 0) {
                        fprintf(stderr, "\x1b[1;31mError writing from file %s. Reason: %s\x1b[0m\n", chats[i].filename, strerror(errno));
                        pthread_mutex_unlock(&chatMutex);
                        exit(1);
                    }
                }
                pthread_mutex_unlock(&chatMutex);
                
                users[user->userID].chats[i] = true;
                users[user->userID].chats[0] = false;
            }
        }
    } else if (strcmp(command, "@leave") == 0) {
        //this user leaves this chatroom, chatroom stays as is
        if (arg == NULL) {
            strcpy(clientMSG, "ERROR: You didn't enter an argument!\n");
        } else {
            name = arg;
            int i = 0;
            while ((strcmp(name, chats[i].name) != 0) && (i < MAXCHATS)) {
                i++;
            }
            if (i == MAXCHATS) {
                //we didn't find chatroom
                strcpy(clientMSG, "ERROR: We can't find this chatroom!\n");
            } else if (users[user->userID].chats[i] == false) {
                sprintf(clientMSG, "ERROR: You aren't in chatroom %s!\n", chats[i].name);
            } else {
                //leave chatroom
                sprintf(clientMSG, "You have left chatroom %s!\n", chats[i].name);
                users[user->userID].chats[i] = false;
                close(chats[i].fd);
                i = 1;
                while ((users[user->userID].chats[i] == false) && (i < MAXCHATS)) {
                    i++;
                }
                if (i == MAXCHATS) {
                    users[user->userID].chats[0] = true;
                }
            }
        }
    } else if (strcmp(command, "@remove") == 0) {
        //throw this chatroom away. check to make sure no users are in it first
        if (arg == NULL) {
            strcpy(clientMSG, "ERROR: You didn't enter an argument!\n");
        } else if (strcmp(arg, "defaultchat") == 0) {
            strcpy(clientMSG, "ERROR: The default chatroom cannot be removed!\n");
        } else {
            name = arg;
            int i = 0;
            while ((strcmp(name, chats[i].name) != 0) && (i < MAXCHATS)) {
                i++;
            }
            if (i == MAXCHATS) {
                //we didn't find chatroom
                strcpy(clientMSG, "ERROR: We can't find this chatroom!\n");
            } else {
                //check chatroom
                int counter = 0;
                pthread_mutex_lock(&chatMutex);
                for (j = 0; j < MAXUSERS; j++) {
                    if (users[j].chats[i] == false) {
                        counter++;
                    } else {
                        printf("user in chat\n");
                    }
                }
                if (counter == MAXUSERS) {
                    users[user->userID].chats[i] = false;
                    if (remove(chats[i].filename) != 0) {
                        fprintf(stderr, "\x1b[1;31mError deleting file %s. Reason: %s\x1b[0m\n", chats[i].filename, strerror(errno));
                        sprintf(clientMSG, "\x1b[1;31mError deleting file %s. Reason: %s\x1b[0m\n", chats[i].filename, strerror(errno));
                    }
                    chatcount--;
                    sprintf(clientMSG, "Chatroom %s removed!\n", chats[i].name);
                    chats[i].name[0] = '\0';
                } else {
                    sprintf(clientMSG, "ERROR: Users are still in chat %s!\n", chats[i].name);
                }
                pthread_mutex_unlock(&chatMutex);
            }
        }
    } else if (strncmp(command, "@list", 4) == 0) {
        if (arg == NULL) {
            strcpy(clientMSG, "ERROR: You didn't enter an argument!\n");
        } else if (strncmp(arg, "users", 5) == 0) {
            i = 0;
            while ((i < MAXUSERS) && (users[i].username[0] != '\0')) {
                sprintf(clientMSG2, "\nUSER %d: %s in chats: ", i + 1, users[i].username);
                write(user->fd, clientMSG2, strlen(clientMSG2));
                for (j = 0; j < MAXCHATS; j++) {
                    if (users[i].chats[j] == true) {
                        sprintf(clientMSG2, "%s ", chats[j].name);
                        write(user->fd, clientMSG2, strlen(clientMSG2));
                    }
                }
                strcpy(clientMSG, "\n");
                i++;
            }
        } else if (strncmp(arg, "chats", 5) == 0) {
            i = 0;
            while ((i < MAXCHATS) && (chats[i].name[0] != '\0')) {
                sprintf(clientMSG2, "CHAT %d: %s\n", i + 1, chats[i].name);
                write(user->fd, clientMSG2, strlen(clientMSG2));
                i++;
            }
            strcpy(clientMSG, "");
        } else if (strncmp(arg, "mychats", 7) == 0) {
            i = 0;
            strcpy(clientMSG2, "You are in chats: ");
            write(user->fd, clientMSG2, strlen(clientMSG2));
            while (i < MAXCHATS) {
                if (users[user->userID].chats[i] == true) {
                    sprintf(clientMSG2, "%s ", chats[i].name);
                    write(user->fd, clientMSG2, strlen(clientMSG2));
                }
                i++;
            }
            strcpy(clientMSG, "\n");
        } else {
            strcpy(clientMSG, "ERROR: Argument must be <users> or <chats>!\n");
        }
    } else if (strncmp(command, "@help", 4) == 0) {
        sprintf(clientMSG, "Command Syntax: \n\t@enter <chatroom>\n\t@enter <chatroom>\n\t@leave <chatroom>\n\t@remove <chatroom>\n\t@list <users>, <chats>, or <mychats>\n\t@help\n\t@exit\n");
    } else if (strcmp(command, "@exit") == 0) {
        //terminate client/server connection
        printf("Closing user->fd %d\n", user->fd);
        close(user->fd);
        pthread_mutex_lock(&chatMutex);
        user->fd = -1;
        user->username[0] = '\0';
        pthread_mutex_unlock(&chatMutex);
        pthread_exit(0);
    } else {
        strcpy(clientMSG, "Not a valid command!\n");
    }
    return clientMSG;
}