//////////////////////////////////////////////////////////////
/*                                                          */
/*      Project for ISA course - HTTP bulletin board        */
/*      Source file for client part                         */
/*                                                          */
/*      Author: Marek Varga                                 */
/*      Login: xvarga14                                     */
/*                                                          */
//////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <ctype.h>

#include "isaclient.h"
//#include "utils.h"
#include "../utils.h"

int main(int argc, char* argv[]) {
    int clientSocket = 0;
    int portNumber = 0;
    hostname = (char*) malloc(sizeof(char) * 1);
    hostname[0] = '\0';
    command = (char*) malloc(sizeof(char) * 1);
    command[0] = '\0';
    struct hostent server;
    struct sockaddr_in serverAddress;

    atexit(atExitFunction);

    // get arguments
    getArgs(argc, argv, &portNumber);

    // find server
    findServer(&portNumber, &server, &serverAddress, &clientSocket);

    // send request and get response
    initiateCommunication(&clientSocket, serverAddress, portNumber);

    exit(EXIT_CODE_0);
}

/**
 * Function deletes all the boards whenever exit() is called.
 */
void atExitFunction() {
    free(hostname);
    free(command);
}

/**
 * Function for getting arguments.
 *
 * @param argc argument count
 * @param argv pointer to array of chars
 * @param portNumber port number to be assigned
 */
void getArgs(int argc, char* argv[], int* portNumber) {
    int arguments;
    opterr = 0;
    char* commandOption = (char*) malloc(sizeof(char) *1);
    commandOption[0] = '\0';
    char* tmp = (char*) malloc(sizeof(char) *1);
    tmp[0] = '\0';

    while ((arguments = getopt(argc, argv, "hH:p:")) != -1 ) {
        switch (arguments) {
            case 'h':
                fprintf(stdout, "Client part for ISA project; try to run with \"-H <host> -p <portnumber> <command>\" arguments\n"
                                "<command> can be any of:\n"
                                "boards - GET /boards\n"
                                "boards add <name> - POST /boards/<name>\n"
                                "board delete <name> - DELETE /boards/<name>\n"
                                "boards list <name> - GET /board/<name>\n"
                                "item add <name> <content> - POST /board/<name>\n"
                                "item delete <name> <id> - DELETE /board/<name>/<id>\n"
                                "item update <name> <id> <content> - PUT /board/<name>/<id>\n"
                                "e.g. ./isaclient -H localhost -p 1025 boards // this will send GET request to the server to obtain /boards\n"
                                "where <name> must be alpha numeric character");
                exit(EXIT_CODE_1);
            case 'H':
                hostname = realloc(hostname, sizeof(char) * (strlen(optarg) + 1));
                strcpy(hostname,optarg);
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
    commandOption = realloc(commandOption, sizeof(char) * (strlen(argv[optind]) + 1));
    strcpy(commandOption, argv[optind]);
    // each addition command needs to be added with ' '
    int i = optind + 1;
    while (i < argc) {
        tmp = realloc(tmp, sizeof(char) * (strlen(commandOption) + 1));
        strcpy(tmp, commandOption);
        commandOption = realloc(commandOption, sizeof(char) * (strlen(tmp) + strlen(" ") + strlen(argv[i]) + 1));
        strcpy(commandOption, tmp);
        strcat(commandOption, " ");
        strcat(commandOption, argv[i]);
        i++;
    }
    // check new line char
    checkNewLineChar(&commandOption);
    command = realloc(command, sizeof(char) * (strlen(commandOption) + 1));
    strcpy(command, commandOption);

    // free allocated helper variables
    free(tmp);
    free(commandOption);
}

/**
 * Function checks whether the new line character is properly represented in command line option.
 * For example, "\\n" is replaced with "\n"
 *
 * @param commandOption char** pointer to string is command option loaded from command line
 */
void checkNewLineChar(char** commandOption) {
    char* tmpString = malloc(sizeof(char) * (strlen(*commandOption) + 1));
    strcpy(tmpString, *commandOption);
    char* result;

    while ((result = strstr(tmpString, "\\n")) != NULL) {
        int count = (int) (strlen(tmpString) - strlen(result));
        char* newBeginning = malloc(sizeof(char) * (count + 1));
        for (int i = 0; i < count; i++) {
            newBeginning[i] = tmpString[i];
        }
        newBeginning[count] = '\0';
        char* tmpResult = malloc(sizeof(char) * (strlen(result) + 1));
        strcpy(tmpResult, result+1);
        tmpResult[0] = '\n';
        char* tmp = malloc(sizeof(char) * (strlen(newBeginning) + 1));
        strcpy(tmp, newBeginning);
        newBeginning = realloc(newBeginning, sizeof(char) * (strlen(tmp) + strlen(tmpResult) + 1));
        strcpy(newBeginning, tmp);
        strcat(newBeginning, tmpResult);
        tmpString = realloc(tmpString, sizeof(char) * (strlen(newBeginning) + 1));
        strcpy(tmpString, newBeginning);
        free(tmp);
        free(tmpResult);
        free(newBeginning);
    }

    free(result);
    *commandOption = realloc(*commandOption, sizeof(char) * (strlen(tmpString) + 1));
    strcpy(*commandOption, tmpString);
    free(tmpString);
}

/**
 * Function tries to find appropriate API service for command arguments or it results in error if no API matches the commands.
 *
 * @param portNumber int server port number
 * @param httpRequest char** pointer to char* representing HTTP header that will be prepared
 */
void prepareHttpRequest(int portNumber, char **httpRequest) {
    char space = ' ';
    char* name = (char*) malloc(sizeof(char) * 1);
    name[0] = '\0';
    int nonAlphaNumeric = 0;

    if (strstr(command, "item update") != NULL) {   // PUT /board
        if (countChar(command, space) >= 3) {
            char* id = (char*) malloc(sizeof(char) * 1);
            id[0] = '\0';
            char* content = (char*) malloc(sizeof(char) * 1);
            content[0] = '\0';
            parseCommandForNameAndIdAndContent(&name, &id, &content);
            concatNameAndId(name, id, &name);
            createRequestWithBody(httpRequest, PUT_BOARD, name, portNumber, content);
            free(content);
            free(id);
            free(name);
            return;
        }
    } else if (strstr(command, "item delete") != NULL) {    // DELETE /board
        if (countChar(command, space) == 3) {
            char* id = (char*) malloc(sizeof(char) * 1);
            id[0] = '\0';
            parseCommandForNameAndId(&name, &id);
            concatNameAndId(name, id, &name);
            createRequestWithoutBody(httpRequest, DELETE_BOARD, name, portNumber);
            free(id);
            free(name);
            return;
        }
    } else if (strstr(command, "item add") != NULL) {       // POST /board
        if (countChar(command, space) >= 2) {
            char* content = (char*) malloc(sizeof(char) * 1);
            content[0] = '\0';
            parseCommandForNameAndContent(&name, &content);
            createRequestWithBody(httpRequest, POST_BOARD, name, portNumber, content);
            free(content);
            free(name);
            return;
        }
    } else if (strstr(command, "board list") != NULL) {    // GET /board
        if (countChar(command, space) == 2) {
            parseCommandForName(&name);
            createRequestWithoutBody(httpRequest, GET_BOARD, name, portNumber);
            free(name);
            return;
        }
    } else if (strstr(command, "board delete") != NULL) {   // DELETE /boards
        if (countChar(command, space) == 2) {
            parseCommandForName(&name);
            createRequestWithoutBody(httpRequest, DELETE_BOARDS, name, portNumber);
            free(name);
            return;
        }
    } else if (strstr(command, "board add") != NULL) {      // POST /boards
        if (countChar(command, space) == 2) {
            parseCommandForName(&name);
            if (isBoarNameAlphaNumeric(name) == 1) {
                createRequestWithoutBody(httpRequest, POST_BOARDS, name, portNumber);
                free(name);
                return;
            }
            // board name contains non-alpha numeric values
            nonAlphaNumeric = 1;
        }
    } else if (strstr(command, "boards") != NULL) {         // GET /boards
        if (countChar(command, space) == 0) {
            createRequestWithoutBody(httpRequest, GET_BOARDS, "", portNumber);
            free(name);
            return;
        }
    }

    free(name);
    free(*httpRequest);
    if (nonAlphaNumeric == 1) {
        fprintf(stderr, "<name> argument can only contain alpha numeric chars; try running with \"-h\" argument for more info.\n");
    } else {
        fprintf(stderr, "Bad <command> argument; try running with \"-h\" argument for more info.\n");
    }
    exit(EXIT_CODE_1);
}

/**
 * Function parses <command> for name.
 *
 * @param name char* parsed name
 */
void parseCommandForName(char** name) {
    // todo check whether <command> has good format, e.g. board add <name>
    int numberOfSpaces = 0;
    char* tmpName = (char*) malloc(sizeof(char) * (strlen(*name) + 1));
    strcpy(tmpName, *name);
    char* tmp = (char*) malloc(sizeof(char) * 1);
    tmp[0] = '\0';

    for (int i = 0; i < (int) strlen(command); i++) {
        if (numberOfSpaces >= 2) {
            // <name> part of command is located behind second space
            if (command[i] != '\0') {
                tmp = realloc(tmp, sizeof(char) * (strlen(tmpName) + 2));
                strcpy(tmp, tmpName);
                tmp[strlen(tmpName)] = command[i];
                tmp[strlen(tmpName)+1] = '\0';
                tmpName = realloc(tmpName, sizeof(char) * (strlen(tmp) + 1));
                strcpy(tmpName, tmp);
            }
        } else{
            // look for space characters
            if (command[i] == ' ') {
                numberOfSpaces++;
            }
        }
    }

    *name = realloc(*name, sizeof(char) * (strlen(tmpName) + 1));
    strcpy(*name, tmpName);

    free(tmp);
    free(tmpName);
}

/**
 * Function parses <command> from argument for name and id.
 *
 * @param name char* parsed name
 * @param id char* parsed id
 */
void parseCommandForNameAndId(char **name, char **id) {
    int numberOfSpaces = 0;
    char* tmpName = (char*) malloc(sizeof(char) * (strlen(*name) + 1));
    strcpy(tmpName, *name);
    char* tmpId = (char*) malloc(sizeof(char) * (strlen(*id) + 1));
    strcpy(tmpId, *id);
    char* tmp = (char*)  malloc(sizeof(char) * 1);
    tmp[0] = '\0';

    for (int i = 0; i < (int) strlen(command); i++) {
        if (command[i] == ' ') {
            numberOfSpaces++;
            continue;
        }
        if (numberOfSpaces >= 3) {
            // <id> part of command is located behind third space
            if (command[i] != '\0') {
                tmp = realloc(tmp, sizeof(char) * (strlen(tmpId) + 2));
                strcpy(tmp, tmpId);
                tmp[strlen(tmpId)] = command[i];
                tmp[strlen(tmpId)+1] = '\0';
                tmpId = realloc(tmpId, sizeof(char) * (strlen(tmp) + 1));
                strcpy(tmpId, tmp);
            }
        } else if (numberOfSpaces >= 2) {
            // <name> part of command is located behind second space
            if (command[i] != '\0') {
                tmp = realloc(tmp, sizeof(char) * (strlen(tmpName) + 2));
                strcpy(tmp, tmpName);
                tmp[strlen(tmpName)] = command[i];
                tmp[strlen(tmpName)+1] = '\0';
                tmpName = realloc(tmpName, sizeof(char) * (strlen(tmp) + 1));
                strcpy(tmpName, tmp);
            }
        }
    }

    *name = realloc(*name, sizeof(char) * (strlen(tmpName) + 1));
    strcpy(*name, tmpName);
    *id = realloc(*id, sizeof(char) * (strlen(tmpId) + 1));
    strcpy(*id, tmpId);

    free(tmp);
    free(tmpId);
    free(tmpName);
}

/**
 * Function parses <command> from argument for name, id and content
 *
 * @param name char* parsed name
 * @param id char* parsed id
 * @param content char* parsed content
 */
void parseCommandForNameAndIdAndContent(char **name, char **id, char** content) {
    int numberOfSpaces = 0;
    char* tmpName = (char*) malloc(sizeof(char) * (strlen(*name) + 1));
    strcpy(tmpName, *name);
    char* tmpId = (char*) malloc(sizeof(char) * (strlen(*id) + 1));
    strcpy(tmpId, *id);
    char* tmpContent = (char*) malloc(sizeof(char) * (strlen(*content) + 1));
    strcpy(tmpContent, *content);
    char* tmp = (char*)  malloc(sizeof(char) * 1);
    tmp[0] = '\0';

    for (int i = 0; i < (int) strlen(command); i++) {
        if (command[i] == ' ') {
            numberOfSpaces++;
            if (numberOfSpaces < 5) {
                continue;
            }
        }
        if (numberOfSpaces >= 4) {
            // <content> part of command is located behind fourth space
            if (command[i] != '\0') {
                tmp = realloc(tmp, sizeof(char) * (strlen(tmpContent) + 2));
                strcpy(tmp, tmpContent);
                tmp[strlen(tmpContent)] = command[i];
                tmp[strlen(tmpContent)+1] = '\0';
                tmpContent = realloc(tmpContent, sizeof(char) * (strlen(tmp) + 1));
                strcpy(tmpContent, tmp);
            }
        } else if (numberOfSpaces >= 3) {
            // <id> part of command is located behind third space
            if (command[i] != '\0') {
                tmp = realloc(tmp, sizeof(char) * (strlen(tmpId) + 2));
                strcpy(tmp, tmpId);
                tmp[strlen(tmpId)] = command[i];
                tmp[strlen(tmpId)+1] = '\0';
                tmpId = realloc(tmpId, sizeof(char) * (strlen(tmp) + 1));
                strcpy(tmpId, tmp);
            }
        } else if (numberOfSpaces >= 2) {
            // <name> part of command is located behind second space
            if (command[i] != '\0') {
                tmp = realloc(tmp, sizeof(char) * (strlen(tmpName) + 2));
                strcpy(tmp, tmpName);
                tmp[strlen(tmpName)] = command[i];
                tmp[strlen(tmpName)+1] = '\0';
                tmpName = realloc(tmpName, sizeof(char) * (strlen(tmp) + 1));
                strcpy(tmpName, tmp);
            }
        }
    }

    *name = realloc(*name, sizeof(char) * (strlen(tmpName) + 1));
    strcpy(*name, tmpName);
    *id = realloc(*id, sizeof(char) * (strlen(tmpId) + 1));
    strcpy(*id, tmpId);
    *content = realloc(*content, sizeof(char) * (strlen(tmpContent) + 1));
    strcpy(*content, tmpContent);

    free(tmp);
    free(tmpContent);
    free(tmpId);
    free(tmpName);
}

/**
 * Function parses <command> from argument for name and content.
 *
 * @param name char* parsed name
 * @param content char* parsed content
 */
void parseCommandForNameAndContent(char** name, char** content) {
    int numberOfSpaces = 0;
    char* tmpName = (char*) malloc(sizeof(char) * (strlen(*name) + 1));
    strcpy(tmpName, *name);
    char* tmpContent = (char*) malloc(sizeof(char) * (strlen(*content) + 1));
    strcpy(tmpContent, *content);
    char* tmp = (char*)  malloc(sizeof(char) * 1);
    tmp[0] = '\0';

    for (int i = 0; i < (int) strlen(command); i++) {
        if (command[i] == ' ') {
            numberOfSpaces++;
            if (numberOfSpaces < 4) {
                continue;
            }
        }
        if (numberOfSpaces >= 3) {
            // <content> part of command is located behind third space
            if (command[i] != '\0') {
                tmp = realloc(tmp, sizeof(char) * (strlen(tmpContent) + 2));
                strcpy(tmp, tmpContent);
                tmp[strlen(tmpContent)] = command[i];
                tmp[strlen(tmpContent)+1] = '\0';
                tmpContent = realloc(tmpContent, sizeof(char) * (strlen(tmp) + 1));
                strcpy(tmpContent, tmp);
            }
        } else if (numberOfSpaces >= 2) {
            // <name> part of command is located behind second space
            if (command[i] != '\0') {
                tmp = realloc(tmp, sizeof(char) * (strlen(tmpName) + 2));
                strcpy(tmp, tmpName);
                tmp[strlen(tmpName)] = command[i];
                tmp[strlen(tmpName)+1] = '\0';
                tmpName = realloc(tmpName, sizeof(char) * (strlen(tmp) + 1));
                strcpy(tmpName, tmp);
            }
        }
    }

    *name = realloc(*name, sizeof(char) * (strlen(tmpName) + 1));
    strcpy(*name, tmpName);
    *content = realloc(*content, sizeof(char) * (strlen(tmpContent) + 1));
    strcpy(*content, tmpContent);

    free(tmp);
    free(tmpContent);
    free(tmpName);
}

/**
 * Function builds HTTP request without body.
 * Request header contains method, board address(url), protocol version and hostname.
 *
 * @param httpRequest char** pointer to request that will be build
 * @param method char* HTTP request header method
 * @param url char* HTTP request header url
 * @param portNumber int server port number
 */
void createRequestWithoutBody(char **httpRequest, char *method, char *url, int portNumber) {
    unsigned long methodLen = strlen(method);
    unsigned long urlLen = strlen(url);
    unsigned long hostnameLen = strlen(hostname);
    char* host = (char*) malloc(sizeof(char) * (strlen("Host: ") + 1));
    strcpy(host, "Host: ");
    unsigned long hostLen = strlen(host);
    char* portNumberAsString = (char*) malloc(sizeof(char) * 1);
    portNumberAsString[0] = '\0';
    intToString(portNumber, &portNumberAsString);
    unsigned long requestSize = methodLen + urlLen + strlen(HTTP_VERSION) + hostnameLen + hostLen + strlen(portNumberAsString) + 9; // space, \r, \n, :, \r, \n, \r, \n, \0

    char* tmpRequest = (char*) malloc(sizeof(char) * (requestSize + 1));
    bzero(tmpRequest, requestSize);
    if (tmpRequest == NULL) {
        exit(EXIT_CODE_1);
    }

    strcpy(tmpRequest, method);
    strcat(tmpRequest, url);
    tmpRequest[methodLen + urlLen] = ' ';
    strcat(tmpRequest, HTTP_VERSION);
    strcat(tmpRequest, "\r\n");
    strcat(tmpRequest, host);
    strcat(tmpRequest, hostname);
    tmpRequest[methodLen + urlLen + strlen(HTTP_VERSION) + 2 + hostLen + hostnameLen] = ':';
    tmpRequest[methodLen + urlLen + strlen(HTTP_VERSION) + 2 + hostLen + hostnameLen + 1] = '\0';
    strcat(tmpRequest, "\r\n\r\n");
    tmpRequest[requestSize] = '\0';

    *httpRequest = realloc(*httpRequest, sizeof(char) * (strlen(tmpRequest) + 1));
    strcpy(*httpRequest, tmpRequest);

    free(portNumberAsString);
    free(tmpRequest);
    free(host);
}

/**
 * Function build HTTP request with body.
 * Request header contains method, board address(url), protocol version, hostname, content-type and content-length.
 *
 * @param httpRequest char** pointer to header that will be build
 * @param method char* HTTP request header method
 * @param url char* HTTP request header url
 * @param portNumber int server port number
 * @param content char* body of HTTP request
 */
void createRequestWithBody(char **httpRequest, char *method, char *url, int portNumber, char *content) {
    unsigned long methodLen = strlen(method);
    unsigned long urlLen = strlen(url);
    unsigned long hostnameLen = strlen(hostname);
    char* host = (char*) malloc(sizeof(char) * (strlen("Host: ") + 1));
    strcpy(host, "Host: ");
    unsigned long hostLen = strlen(host);
    unsigned long contentLen = strlen(content);
    char contentLenAsString[12];
    snprintf(contentLenAsString, 12, "%lu", contentLen);
    unsigned long contentTypeLen = strlen(CONTENT_TYPE);
    unsigned long contentLenghtLen = strlen(CONTENT_LENGTH);
    char* contentLengthWithLengthOfContent = (char*) malloc(sizeof(char) * (contentLenghtLen + strlen(contentLenAsString) + 1));
    strcpy(contentLengthWithLengthOfContent, CONTENT_LENGTH);
    strcat(contentLengthWithLengthOfContent, contentLenAsString);
    char* portNumberAsString = (char*) malloc(sizeof(char) * 1);
    portNumberAsString[0] = '\0';
    intToString(portNumber, &portNumberAsString);
    unsigned long requestSize = methodLen + urlLen + strlen(HTTP_VERSION) + hostnameLen + hostLen + strlen(portNumberAsString) + contentTypeLen + strlen(contentLengthWithLengthOfContent) + contentLen + 13; // space, \r, \n, :, \r, \n, \r, \n, \r, \n, \r, \n, \0

    char* tmpRequest = (char*) malloc(sizeof(char) * (requestSize + 1));
    bzero(tmpRequest, requestSize);
    if (tmpRequest == NULL) {
        exit(EXIT_CODE_1);
    }

    strcpy(tmpRequest, method);
    strcat(tmpRequest, url);
    tmpRequest[methodLen + urlLen] = ' ';
    strcat(tmpRequest, HTTP_VERSION);
    strcat(tmpRequest, "\r\n");
    strcat(tmpRequest, host);
    strcat(tmpRequest, hostname);
    tmpRequest[methodLen + urlLen + strlen(HTTP_VERSION) + 2 + hostLen + hostnameLen] = ':';
    tmpRequest[methodLen + urlLen + strlen(HTTP_VERSION) + 2 + hostLen + hostnameLen + 1] = '\0';
    strcat(tmpRequest, portNumberAsString);
    strcat(tmpRequest, "\r\n");
    strcat(tmpRequest, CONTENT_TYPE);
    strcat(tmpRequest, "\r\n");
    strcat(tmpRequest, contentLengthWithLengthOfContent);
    strcat(tmpRequest, "\r\n");
    strcat(tmpRequest, "\r\n"); // blank line
    // body
    strcat(tmpRequest, content);
    tmpRequest[requestSize] = '\0';

    *httpRequest = realloc(*httpRequest, sizeof(char) * (strlen(tmpRequest) + 1));
    strcpy(*httpRequest, tmpRequest);

    free(contentLengthWithLengthOfContent);
    free(tmpRequest);
    free(host);
    free(portNumberAsString);
}

/**
 * Function finds server according to hostname argument (either name or IPv4). Function also creates socket.
 *
 * @param portNumber port number of server
 * @param server structure with info about host
 * @param serverAddress structure with socket elements
 * @param clientSocket descriptor of client socket
 */
void findServer(const int *portNumber, struct hostent* server, struct sockaddr_in *serverAddress, int* clientSocket) {
    // "
    if ( (server=gethostbyname(hostname)) == NULL) {
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
 * @param portNumber int number of port that the server is running on
 */
void initiateCommunication(const int *clientSocket, struct sockaddr_in serverAddress, int portNumber) {
    char* httpRequest = (char*) malloc(sizeof(char) * 1);
    httpRequest[0] = '\0';
    prepareHttpRequest(portNumber, &httpRequest);

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
        free(httpRequest);
        exit(1);
    }
    // " inspired by: Rysavy Ondrej - DemoTcp/client.c

    // prepare request for sending
    strcpy(request, httpRequest);

    // send request to server
    if ( write(*clientSocket, request, strlen(request)) < 0) {
        fprintf(stderr, "Writing to socket resulted in error.\n");
        close(*clientSocket);
        free(httpRequest);
        exit(1);
    }

    // read response
    if (read(*clientSocket, response, BUFSIZ) < 0) {
        fprintf(stderr, "Reading from socket resulted in error.\n");
        close(*clientSocket);
        free(httpRequest);
        exit(EXIT_CODE_1);
    }

    char* tmpHeader = (char*) malloc(sizeof(char) * 1);
    tmpHeader[0] = '\0';
    char* tmpBody = (char*) malloc(sizeof(char) * 1);
    tmpBody[0] = '\0';
    parseResponse(response, &tmpHeader, &tmpBody);

    // print header (without blank line) and content
    fprintf(stderr, "%s\n", tmpHeader);
    fprintf(stdout, "%s\n", tmpBody);

    free(tmpHeader);
    free(tmpBody);
    free(httpRequest);
    close(*clientSocket);
}

/**
 * Function parses HTTP response to header and body.
 * Firstly header is separated from the response by locating blank line in the message.
 * Body can be empty or contain message from server.
 *
 * @param response char* HTTP response
 * @param header pointer to char* where separated header from the response will be stored
 * @param body  pointer to char* where separated body from the response will be stored
 *
 * @return 1 on success, 0 when an error occurred
 */
int parseResponse(char* response, char** header, char** body) {
    // check if blank line exists
    char* headerEnd = strstr(response, "\r\n\r\n");  // header and content is separated by blank line
    if (headerEnd == NULL) {
        fprintf(stderr, "Response is badly formatted");
        return 0;
    }

    // separate header from content
    int headerLen = (int) (strlen(response) - strlen(headerEnd));
    char* tmpHeader = (char*) malloc(sizeof(char) * (headerLen + 3)); // \r, \n, \0
    bzero(tmpHeader, headerLen + 3);
    for (int i = 0; i < headerLen; i++) {
        tmpHeader[i] = response[i];
    }
    tmpHeader[headerLen] = '\r';
    tmpHeader[headerLen+1] = '\n';
    tmpHeader[headerLen+2] = '\0';
    *header = realloc(*header, sizeof(char) * (strlen(tmpHeader) + 1));
    strcpy(*header, tmpHeader);

    // parse header
    int responseCode = 0;
    int contentLenght = 0;
    int parseResult = parseHeader(tmpHeader, &responseCode, &contentLenght);

    // get content
    int contentLen = (int) (strlen(response) - strlen(tmpHeader) - 2);   // content is located behind blank line; \r\n
    if (contentLen > 0) {
        if (contentLen != contentLenght) {  // compare length of body and Content-Length parameter
            fprintf(stderr, "Content-Length and length of HTTP message body have different length");
            parseResult = 0;
        } else {
            char *tmpContent = (char *) malloc(sizeof(char) * (contentLen + 1));
            bzero(tmpContent, contentLen + 1);
            strncpy(tmpContent, response + strlen(tmpHeader) + 2, contentLen);
            *body = realloc(*body, sizeof(char) * (strlen(tmpContent) + 1));
            strcpy(*body, tmpContent);
            free(tmpContent);
        }
    }

    free(tmpHeader);
    return parseResult;
}

/**
 * Function parses HTTP response header.
 * Protocol version, status code, content-type, content-length are looked for.
 *
 * @param request char* received HTTP response
 * @param responseCode int* where response code from response will be stored
 * @param contentLength int* where the lenght of the reponse body will be stored
 *
 * @return 1 on success, 0 if an error occurred
 */
int parseHeader(char* request, int* responseCode, int* contentLength) {
    int numberOfSpaces = 0;
    char* tmpProtocolVersion = (char*) malloc(sizeof(char) * 1);
    tmpProtocolVersion[0] = '\0';
    char* tmpStatusCode = (char*) malloc(sizeof(char) * 1);
    tmpStatusCode[0] = '\0';
    char* tmpReasonPhrase = (char*) malloc(sizeof(char) * 1);
    tmpReasonPhrase[0] = '\0';
    int parseSuccess = 1;

    // parse header to Protocol version and Status code
    for (int i = 0; i < (int) strlen(request); i++) {
        if (request[i] == ' ' || (request[i] == '\r' && (i+1 < (int) strlen(request)) && request[i+1] == '\n')) {
            numberOfSpaces++;
            continue;
        }

        if (numberOfSpaces == 0) {          // Protocol version is before first space
            addCharToString(&tmpProtocolVersion, request[i]);
        } else if (numberOfSpaces == 1) {   // Status code is behind first space
            addCharToString(&tmpStatusCode, request[i]);
        } /*else if (numberOfSpaces == 2) {   // Reason phrase is behind second space
            addCharToString(&tmpReasonPhrase, request[i]);
        }*/
    }

    // check either the both of Content-type, Content-length are present or neither of them
    if (strstr(request, CONTENT_TYPE) != NULL) {
        if (strstr(request, CONTENT_LENGTH) == NULL) {
            fprintf(stderr, "Content-Length is absent in HTTP response header\n");
            parseSuccess = 0;
        } else {
            char* contentLengthFromResponse = strstr(request, CONTENT_LENGTH);
            long tmp = strtol(contentLengthFromResponse+strlen(CONTENT_LENGTH), NULL, 10);
            *contentLength = (int) tmp;
        }
    } else {
        if (strstr(request, CONTENT_LENGTH) != NULL) {
            fprintf(stderr, "Content-Type is absent or is not of text/plain type in HTTP response header\n");
            parseSuccess = 0;
        }
    }

    // check if Protocol version is HTTP/1.1
    if (strcmp(tmpProtocolVersion, HTTP_VERSION) != 0) {
        fprintf(stderr, "Bad protocol version; received=%s;  expected=%s\n", tmpProtocolVersion, HTTP_VERSION);
        parseSuccess = 0;
    }

    // change response code from string to int
    long code = strtol(tmpStatusCode, NULL, 10);
    *responseCode = (int) code;

    free(tmpReasonPhrase);
    free(tmpProtocolVersion);
    free(tmpStatusCode);

    return parseSuccess;
}