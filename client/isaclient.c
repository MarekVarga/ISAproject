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

#include "isaclient.h"


// function checks and gets arguments
void getArgs(int argc, char* argv[], int* portNumber, char** hostname, char** login);

ApiService findApiService(char** command);

int countChar(char* haystack, const char needle);

// function finds server and creates socket
void findServer(char **hostname, const int *portNumber, struct hostent* server, struct sockaddr_in *serverAddress, int *clientSocket);

// function connects to server, sends requests and prints responses
void initiateCommunication(const int *clientSocket, struct sockaddr_in serverAddress, const int* n, const int* f, const int* l, char** login);

// function checks format of messages send through sockets
int checkProtocol(char* message);

int main(int argc, char* argv[]) {
    int clientSocket = 0;
    int portNumber = 0;
    char* hostname = malloc(sizeof(strlen("") + 1));
    char* command = malloc(sizeof(strlen("") + 1));
    struct hostent server;
    struct sockaddr_in serverAddress;

    // get arguments
    getArgs(argc, argv, &portNumber, &hostname, &command);
    findApiService(&command);

    // find server
    findServer(&hostname, &portNumber, &server, &serverAddress, &clientSocket);

    int n = 0;
    int f = 0;
    int l = 0;
    // send request and get response
    initiateCommunication(&clientSocket, serverAddress, &n, &f, &l, &command);

    exit(EXIT_CODE_0);
}

/**
 * Function for getting arguments.
 *
 * @param argc argument count
 * @param argv pointer to array of chars
 * @param portNumber port number to be assigned
 * @param hostname host name to be assigned
 * @param n will be set to 1 if program is run with -n argument
 * @param f will be set to 1 if program is run with -f argument
 * @param l will be set to 1 if program is run with -l argument
 * @param login login to be assigned
 */
void getArgs(int argc, char* argv[], int* portNumber, char** hostname, char** login) {
    int arguments;
    opterr = 0;
    char* commandOption = malloc(sizeof(strlen("") + 1));
    char* tmp = malloc(sizeof(strlen("") + 1));

    while ((arguments = getopt(argc, argv, "hH:p:")) != -1 ) {
        switch (arguments) {
            case 'h':
                fprintf(stdout, "Client part for ISA project; try to run with \"-H <host> -p <portnumber> <command>\" arguments\n"
                                "<command> can be any of:\nboards - GET /boards\nboards add <name> - POST /boards/<name>\n"
                                "board delete <name> - DELETE /boards/<name>\nboards list <name> - GET /board/<name>\n"
                                "item add <name> <content> - POST /board/<name>\nitem delete <name> <id> - DELETE /board/<name>/<id>\n"
                                "item update <name> <id> <content> - PUT /board/<name>/<id>\nbe.g. ./isaclient -H localhost -p 1025 boards // this will send GET request to the server to obtain /boards");
                exit(EXIT_CODE_1);
            case 'H':
                *hostname = realloc(*hostname, sizeof(strlen(optarg) + 1));
                strcpy(*hostname,optarg);
                break;
            case 'p' :
                *portNumber = (int) strtol(optarg,&optarg,10);
                if (*portNumber < 1024 || *portNumber > 65535) {
                    fprintf(stderr, "Option -p requires an argument in interval <1024,65535>; provided: %d.\n", *portNumber);
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

                break;
        }
    }

    // check if number of arguments is correct (minimum is 6)
    if (argc < 6) {
        fprintf(stderr, "Wrong number of parameters, try to run with \"-h\" argument for more details.\n");
        exit(EXIT_CODE_1);
    }

    // find API commands
    // copy first command
    commandOption = realloc(commandOption, sizeof(strlen(argv[optind]) + 1));
    strcpy(commandOption, argv[optind]);
    // each addition command needs to be added with ' '
    int i = optind + 1;
    while (i < argc) {
        tmp = realloc(tmp, sizeof(strlen(commandOption) + 1));
        strcpy(tmp, commandOption);
        commandOption = realloc(commandOption, sizeof(strlen(tmp) + strlen(" ") + strlen(argv[i]) + 1));
        strcpy(commandOption, tmp);
        strcat(commandOption, " ");
        strcat(commandOption, argv[i]);
        i++;
    }
    *login = realloc(*login, sizeof(strlen(commandOption) + 1));
    strcpy(*login, commandOption);

    // free allocated helper variables
    free(tmp);
    free(commandOption);
}

/**
 * Function tries to find appropriate API service for command arguments or it results in error if no API matches the commands.
 *
 * @param command
 *
 * @return appropriate API
 */
ApiService findApiService(char** command) {
    char space = ' ';
    if (strstr(*command, "item update") != NULL) {
        if (countChar(*command, space) == 3) {

        }
    } else if (strstr(*command, "item delete") != NULL) {
        if (countChar(*command, space) == 3) {

        }
    } else if (strstr(*command, "item add") != NULL) {
        if (countChar(*command, space) == 3) {

        }
    } else if (strstr(*command, "boards list") != NULL) {
        if (countChar(*command, space) == 2) {

        }
    } else if (strstr(*command, "board delete") != NULL) {
        if (countChar(*command, space) == 2) {

        }
    } else if (strstr(*command, "board add") != NULL) {
        if (countChar(*command, space) == 2) {

        }
    } else if (strstr(*command, "boards") != NULL) {
        if (countChar(*command, space) == 0) {

        }
    }

    fprintf(stderr, "Bad <command> argument; try running with \"-h\" argument for more info.\n");
    exit(EXIT_CODE_1);
}

/**
 * Function counts the occurrences of certain character (needle) in a string (haystack).
 *
 * @param haystack char whose occurrence is counted
 * @param needle string in which char is counted
 *
 * @return number of occurrences of needle in haystack
 */
int countChar(char* haystack, const char needle) {
    int occurrences = 0;

    for (int i = 0; i < strlen(haystack); i++) {
        if (haystack[i] == needle) {
            occurrences++;
        }
    }

    return occurrences;
}

/**
 * Function finds server according to hostname argument (either name or IPv4). Function also creates socket.
 *
 * @param hostname host name of server
 * @param portNumber port number of server
 * @param server structure with info about host
 * @param serverAddress structure with socket elements
 * @param clientSocket descriptor of client socket
 */
void findServer(char **hostname, const int *portNumber, struct hostent* server, struct sockaddr_in *serverAddress, int* clientSocket) {
    // "
    if ( (server=gethostbyname(*hostname)) == NULL) {
        fprintf(stderr, "There appears to be no such host.\n");
    }

    bzero(serverAddress, sizeof(&serverAddress));
    serverAddress->sin_family = AF_INET;
    bcopy(server->h_addr, (char *)&serverAddress->sin_addr.s_addr, (size_t)server->h_length);
    serverAddress->sin_port = htons(*portNumber);
    // " inspired by: Rysavy Ondrej - DemoTcp/client.c

    // create socket
    if ( (*clientSocket=socket(AF_INET,SOCK_STREAM,0)) < 0) {
        fprintf(stderr, "Creating socket ended with error.\n");
    }
}

/**
 * Function connects to server, sends request to get specific data and receives response.
 *
 * @param clientSocket socket descriptor
 * @param serverAddress socket address structure of server
 * @param n -n argument of program
 * @param f -f argument of program
 * @param l -l argument of program
 * @param login login of sought user
 */
void initiateCommunication(const int *clientSocket, struct sockaddr_in serverAddress, const int* n, const int* f, const int* l, char** login){
    ssize_t r = 0;
    char request[BUFSIZ];
    char response[BUFSIZ];
    char message [BUFSIZ];
    bzero(message,BUFSIZ);
    bzero(request,BUFSIZ);
    bzero(response,BUFSIZ);

// "
    // connect to server
    if ( connect(*clientSocket,(struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        fprintf(stderr, "Connecting to server resulted in error.\n");
        close(*clientSocket);
        exit(1);
    }
// " inspired by: Rysavy Ondrej - DemoTcp/client.c

    // prepare request for sending
    strcpy(request,"**Protocol xvarga14**");
    if ( *n != 0) {
        strcat(request, "n");
        strcat(request, *login);
    } else if ( *f != 0 ) {
        strcat(request, "f");
        strcat(request, *login);
    } else if ( *l != 0 ) {
        strcat(request, "l");
        if ( *login != NULL ) {
            strcat(request, *login);
        }
    }
    strcat(request, "***");

    // send request to server
    if ( write(*clientSocket, request, strlen(request)) < 0) {
        fprintf(stderr, "Writing to socket resulted in error.\n");
        close(*clientSocket);
        exit(1);
    }

    // if client was run with -n or -f argument
    if ( *l == 0) {
        // get response from server
        if (read(*clientSocket, response, BUFSIZ) < 0) {
            fprintf(stderr, "Reading from socket resulted in error.\n");
            close(*clientSocket);
            exit(1);
        }

        // check if communication is in set protocol
        if ( checkProtocol(response) != 0) {
            fprintf(stderr, "Communication in unknown protocol.\n");
            close(*clientSocket);
            exit(1);
        }

        // print response
        memcpy(message, &response[21], strstr(response, "***")-response-21);
        fprintf(stdout, "%s\n", message);

    } else {
        // if client was run with -l argument
        while ( (r = read(*clientSocket,response, BUFSIZ)) >= 0 && (strcmp(response, "**Protocol xvarga14**No more entries.***")) != 0) {
            // check if communication is in set protocol
            if ( checkProtocol(response) != 0) {
                fprintf(stderr, "Communication in unknown protocol.\n");
                close(*clientSocket);
                exit(1);
            }

            // print response
            memcpy(message, &response[21], strstr(response, "***")-response-21);
            fprintf(stdout, "%s\n", message);

            bzero(response, BUFSIZ);
            bzero(message, BUFSIZ);

            // tell server the message was delivered
            if ( write(*clientSocket, "**Protocol xvarga14**Message delivered.***", 256) < 0) {
                fprintf(stderr, "Writing to socket resulted in error.\n");
                close(*clientSocket);
                exit(1);
            }
        }

        // print error if reading from socket was unsuccessful
        if ( r < 0 ) {
            fprintf(stderr, "Reading from socket resulted in error.\n");
            close(*clientSocket);
            exit(1);
        }

    }

    close(*clientSocket);
}

/**
 * Function checks if message format meets the protocol criteria.
 *
 * @param message message to be checked
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