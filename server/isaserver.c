#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <signal.h>
#include <pwd.h>

#include "isaserver.h"

int socketDescriptor;

// function checks and gets arguments
int checkArgs(int argc, char **argv);

// function tries to establish a server
void establishConnection(int portNumber, int *socketDescriptor, struct sockaddr_in* serverAddress);

// function communicates with client
void satisfyClient(const int* clientSocketDescriptor);

int checkProtocol(char *message);

int main(int argc, char* argv[]) {
    int port, clientSocketDescriptor, pid;
    struct sockaddr_in serverAddress, clientAddress;

    // get arguments
    port = checkArgs(argc, argv);

    // make server
    establishConnection(port, &socketDescriptor, &serverAddress);
    // "
    socklen_t clientAddressLength = sizeof(clientAddress);
    // " inspired by: Rysavy Ondrej - DemoTcp/server.c

    while (SIGINT) {
        // call to accept function
        // "
        clientSocketDescriptor = accept(socketDescriptor, (struct sockaddr *) &clientAddress, &clientAddressLength);
        // " inspired by: Rysavy Ondrej - DemoTcp/server.c

        // check if client socket descriptor is valid
        if (clientSocketDescriptor < 0) {
            fprintf(stderr, "Connecting to client resulted in error.\n");
            exit(EXIT_CODE_1);
        }

        // create child process
        pid = fork();

        if (pid < 0) {
            fprintf(stderr, "Fork error");
            close(socketDescriptor);
            close(clientSocketDescriptor);
            exit(EXIT_CODE_1);
        }

        if (pid == 0) {
            // this is child process - child process will begin communicating with client
            // "
            close(socketDescriptor);
            // " inspired by: Rysavy Ondrej - DemoFork/server.c
            satisfyClient(&clientSocketDescriptor);
            // todo close socket
            exit(EXIT_CODE_0);
        } else {
            // this is parent process - parent process closes client socket descriptor and will wait for new connection
            // "
            close(clientSocketDescriptor);
            // " inspired by: Rysavy Ondrej - DemoFork/server.c
        }

    }

    exit(EXIT_CODE_0);
}

/**
 * Function checks arguments with getopt(). On success it returns port number.
 *
 * @param argc argument count
 * @param argv pointer to array of chars
 *
 * @return new port number
 */
int checkArgs(int argc, char **argv) {
    int port = 0;
    int arguments;

    while ((arguments = getopt(argc, argv, "hp:")) != -1) {
        switch (arguments) {
            case 'h' :
                fprintf(stdout, "Server part for ISA project; try to run with \"-p portnumber\" arguments\ne.g. ./isaserver -p 1025");
                exit(EXIT_CODE_1);
            case 'p' :
                port = (int)  strtoll(optarg, &optarg, 10);
                if (port < 1024 || port > 65535) {
                    fprintf(stderr, "Option -p requires an argument in interval <1024, 65535>; provided: %d.\n", port);
                    exit(EXIT_CODE_1);
                }
                break;
            case '?' :
                if (optopt == 'p') {
                    fprintf(stderr, "Option -p requires an argument in interval <1024,65535>.\n");
                }
                else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option -%c.\n", optopt);
                }
                exit(EXIT_CODE_1);
            default:
                fprintf(stderr, "Unknown option -%c.\n", optopt);
                exit(EXIT_CODE_1);
        }
    }

    // to ensure that there is only one -p argument
    if (optind < argc || argc != 3) {
        fprintf(stderr, "Wrong number of arguments; try to run with \"-h\" for more detail.\n");
        exit(EXIT_CODE_1);
    }

    return port;
}

/**
 * Function makes process a server. Function calls these functions to do so: socket(), bind(), listen().
 *
 * @param portNumber port number
 * @param socketDescriptor socket descriptor returned by socket() will be stored here
 * @param serverAddress structure with socket information
 */
void establishConnection(int portNumber, int* socketDescriptor, struct sockaddr_in* serverAddress) {

    // call to socket function
    *socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);

    // check whether socket descriptor is valid
    if(*socketDescriptor < 0){
        fprintf(stderr, "Opening socket resulted in error.\n");
        exit(EXIT_CODE_1);
    }

    // initialize structure to hold socket information
    struct sockaddr_in socketStructure;
    // "
    memset(&socketStructure, 0, sizeof(socketStructure));
    socketStructure.sin_family = AF_INET;
    socketStructure.sin_addr.s_addr = INADDR_ANY;
    socketStructure.sin_port = htons(portNumber);

    // call to bind function
    if ((bind(*socketDescriptor, (struct sockaddr*)&socketStructure, sizeof(socketStructure))) < 0) {
        fprintf(stderr, "Binding resulted in error.\n");
        close(*socketDescriptor);
        exit(EXIT_CODE_1);
    }
    // " inspired by: Rysavy Ondrej - DemoTcp/server.c

    // call to listen function
    if ( listen(*socketDescriptor,10) < 0 ) {
        fprintf(stderr, "Trying to listen resulted in error.\n");
        close(*socketDescriptor);
        exit(EXIT_CODE_1);
    }

    *serverAddress = socketStructure;
}

/**
 * Function receives request from client, checks if request message matches required protocol then tries to meet client's request and sends response.
 *
 * @param clientSocketDescriptor socket descriptor of client
 */
void satisfyClient(const int* clientSocketDescriptor) {

    int found = 0;
    char request[BUFSIZ];
    char requestOption = '\0';
    char login[BUFSIZ];
    char response[BUFSIZ];
    char confirmationMessage[BUFSIZ];
    char tmp[BUFSIZ];
    struct passwd* user;
    bzero(request,BUFSIZ);
    bzero(login, BUFSIZ);
    bzero(response, BUFSIZ);
    bzero(confirmationMessage, BUFSIZ);
    bzero(tmp, BUFSIZ);

    // read request from client
    if ( read(*clientSocketDescriptor,request, BUFSIZ) < 0) {
        fprintf(stderr, "Reading from socket resulted in error.\n");
        close(*clientSocketDescriptor);
        exit(EXIT_CODE_1);
    }

    // check if communication is in set protocol
    if ( checkProtocol(request) != 0) {
        fprintf(stderr, "Communication is in unknown protocol.\n");
        close(*clientSocketDescriptor);
        exit(EXIT_CODE_1);
    }

    // find which option client wants
    size_t pos = strstr(request, "***") - request;
    memcpy(&requestOption, &request[21], 1);
    memcpy(&login, &request[22], pos-22);

    setpwent();
    // if client was run with -n or -f argument
    if (requestOption != 'l') {

        // find user
        user = getpwnam(login);

        if (user != NULL) {
            if (requestOption == 'n') {
                // return name of user according to login
                strcpy(response,"**Protocol xvarga14**");
                strcat(response, user->pw_gecos);
                strcat(response, "***");
            } else if (requestOption == 'f') {
                // return working directory of user according to login
                strcpy(response,"**Protocol xvarga14**");
                strcat(response, user->pw_dir);
                strcat(response, "***");
            }
        } else {
            // inform client that no user was found for given login
            sprintf(response, "**Protocol xvarga14**Entry was not found for given login.***");
        }

        // satisfy client
        if ( write(*clientSocketDescriptor, response, strlen(response)) < 0) {
            fprintf(stderr, "Writing to socket resulted in error.\n");
            close(*clientSocketDescriptor);
            exit(EXIT_CODE_1);
        }
        // if client was run with -l argument
    } else {
        while ( (user = getpwent()) != NULL) {
            memcpy(tmp, user->pw_name, strlen(login));
            // if user name matches login prefix
            if (strcmp(tmp, login) == 0) {
                strcpy(response,"**Protocol xvarga14**");
                strcat(response, user->pw_name);
                strcat(response, "***");
                found++;

                // satisfy client
                if ( write(*clientSocketDescriptor, response, strlen(response)) < 0) {
                    fprintf(stderr, "Writing to socket resulted in error.\n");
                    close(*clientSocketDescriptor);
                    exit(EXIT_CODE_1);
                }

                // wait for client to confirm my response
                if ( read(*clientSocketDescriptor,confirmationMessage, BUFSIZ) < 0) {
                    fprintf(stderr, "Reading from socket resulted in error.\n");
                    close(*clientSocketDescriptor);
                    exit(EXIT_CODE_1);
                }

                // check if client received my response
                if (strcmp(confirmationMessage, "**Protocol xvarga14**Message delivered.***") != 0) {
                    fprintf(stderr, "Client did not receive response.\n");
                    close(*clientSocketDescriptor);
                    exit(EXIT_CODE_1);
                }
            }

            bzero(user, sizeof(&user));
            bzero(tmp, BUFSIZ);
            bzero(confirmationMessage, BUFSIZ);
            bzero(response, BUFSIZ);
        }

        // if no entry was found for given login prefix
        if (found == 0) {
            if ( write(*clientSocketDescriptor, "**Protocol xvarga14**No entry for given login prefix.***", 256) < 0) {
                fprintf(stderr, "Writing to socket resulted in error.\n");
                close(*clientSocketDescriptor);
                exit(EXIT_CODE_1);
            }
        }

        // tell client that there is nothing more to send
        if ( write(*clientSocketDescriptor, "**Protocol xvarga14**No more entries.***", 256) < 0) {
            fprintf(stderr, "Writing to socket resulted in error.\n");
            close(*clientSocketDescriptor);
            exit(EXIT_CODE_1);
        }
    }
    endpwent();
}

/**
 * Function checks if message format meets the protocol criteria.
 *
 * @param message message to be checked
 *
 * @return 0 if message is OK otherwise return 1
 */
int checkProtocol(char* message) {

    char protocolEnding[4];
    bzero(protocolEnding, 4);
    memcpy(&protocolEnding,strstr(message,"***"),strlen("***")+1);
    char protocolOpening[21];
    bzero(protocolOpening, 21);
    memmove(&protocolOpening,message,21);

    if ( strcmp(protocolOpening,"**Protocol xvarga14**") != 0 && strcmp(protocolEnding, "***") != 0 ) {
        return 1;
    }

    return 0;
}