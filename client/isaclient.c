#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <ctype.h>

#include "isaclient.h"
//#include "api.h"
#include "../api.h"

// function checks and gets arguments
void getArgs(int argc, char* argv[], int* portNumber, char** hostname, char** login);

// function prepares http request header
void prepareHttpRequest(char* command, char* hostname, char** httpRequest);

// function counts occurrences of char in char*
int countChar(char* haystack, char needle);

// funciton parses <comman> for name
void parseCommandForName(char* command, char **name);

// funciton parses <command> to name and id
void parseCommandForNameAndId(char* command, char **name, char **id);

// funciton parses <command> to name, id and content
void parseCommandForNameAndIdAndContent(char* command, char** name, char** id, char** content);

// funciton parses <command> to name and content
void parseCommandForNameAndContent(char* command, char** name, char** content);

// function concatenates name and id for api
void concatNameAndId(char* name, char* id);

// function creates HTTP request without body
void createRequestWithoutBody(char** httpRequest, char* method, char* url, char* hostname);

// function creates HTTP request with body
void createRequestWithBody(char** httpRequest, char* method, char* url, char* hostname, char* content);

struct Api* newApi(char* command, char* apiEquivalent, int numberOfCommands);

void deleteApi(struct Api* api);

// function finds server and creates socket
void findServer(char **hostname, const int *portNumber, struct hostent* server, struct sockaddr_in *serverAddress, int *clientSocket);

// function connects to server, sends requests and prints responses
void initiateCommunication(const int *clientSocket, struct sockaddr_in serverAddress, char** apiCommand, char** hostname);

int main(int argc, char* argv[]) {
    int clientSocket = 0;
    int portNumber = 0;
    char* hostname = (char*) malloc(sizeof(char) * 1);
    hostname[0] = '\0';
    char* command = (char*) malloc(sizeof(char) * 1);
    command[0] = '\0';
    struct hostent server;
    struct sockaddr_in serverAddress;

    // get arguments
    getArgs(argc, argv, &portNumber, &hostname, &command);

    // find server
    findServer(&hostname, &portNumber, &server, &serverAddress, &clientSocket);

    // send request and get response
    initiateCommunication(&clientSocket, serverAddress, &command, &hostname);

    free(hostname);
    free(command);
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
    char* commandOption = (char*) malloc(sizeof(char) *1);
    commandOption[0] = '\0';
    char* tmp = (char*) malloc(sizeof(char) *1);
    tmp[0] = '\0';

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
                *hostname = realloc(*hostname, sizeof(char) * (strlen(optarg) + 1));
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
    *login = realloc(*login, sizeof(char) * (strlen(commandOption) + 1));
    strcpy(*login, commandOption);

    // free allocated helper variables
    free(tmp);
    free(commandOption);
}

/**
 * Function tries to find appropriate API service for command arguments or it results in error if no API matches the commands.
 *
 * @param command char* <ommand> that will be parsed and then incluced in header
 * @param hostname char* server hostname
 * @param httpRequest char** pointer to char* representing HTTP header that will be prepared
 */
void prepareHttpRequest(char* command, char* hostname, char** httpRequest) {
    char space = ' ';
    char* name = (char*) malloc(sizeof(char) * 1);
    name[0] = '\0';

    if (strstr(command, "item update") != NULL) {   // PUT /board
        if (countChar(command, space) >= 3) {
            char* id = (char*) malloc(sizeof(char) * 1);
            id[0] = '\0';
            char* content = (char*) malloc(sizeof(char) * 1);
            content[0] = '\0';
            parseCommandForNameAndIdAndContent(command, &name, &id, &content);
            concatNameAndId(name, id);
            createRequestWithBody(httpRequest, PUT_BOARD, name, hostname, content);
            free(content);
            free(id);
            free(name);
            return;
        }
    } else if (strstr(command, "item delete") != NULL) {    // DELETE /board
        if (countChar(command, space) == 3) {
            char* id = (char*) malloc(sizeof(char) * 1);
            id[0] = '\0';
            parseCommandForNameAndId(command, &name, &id);
            concatNameAndId(name, id);
            createRequestWithoutBody(httpRequest, DELETE_BOARD, name, hostname);
            free(id);
            free(name);
            return;
        }
    } else if (strstr(command, "item add") != NULL) {       // POST /board
        if (countChar(command, space) >= 3) {
            char* content = (char*) malloc(sizeof(char) * 1);
            content[0] = '\0';
            parseCommandForNameAndContent(command, &name, &content);
            //createRequestWithoutBody(httpRequest, POST_BOARD, name, hostname);
            createRequestWithBody(httpRequest, POST_BOARD, name, hostname, content);
            free(content);
            free(name);
            return;
        }
    } else if (strstr(command, "boards list") != NULL) {    // GET /board
        if (countChar(command, space) == 2) {
            parseCommandForName(command, &name);
            createRequestWithoutBody(httpRequest, GET_BOARD, name, hostname);
            free(name);
            return;
        }
    } else if (strstr(command, "board delete") != NULL) {   // DELETE /boards
        if (countChar(command, space) == 2) {
            parseCommandForName(command, &name);
            createRequestWithoutBody(httpRequest, DELETE_BOARDS, name, hostname);
            free(name);
            return;
        }
    } else if (strstr(command, "board add") != NULL) {      // POST /boards
        if (countChar(command, space) == 2) {
            parseCommandForName(command, &name);
            createRequestWithoutBody(httpRequest, POST_BOARDS, name, hostname);
            free(name);
            return;
        }
    } else if (strstr(command, "boards") != NULL) {         // GET /boards
        if (countChar(command, space) == 0) {
            createRequestWithoutBody(httpRequest, GET_BOARDS, "", hostname);
            free(name);
            return;
        }
    }

    free(name);
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

    for (int i = 0; i < (int) strlen(haystack); i++) {
        if (haystack[i] == needle) {
            occurrences++;
        }
    }

    return occurrences;
}

/**
 * Function parses <command> for name.
 *
 * @param command char* <command> parsed
 * @param name char* parsed name
 */
void parseCommandForName(char* command, char** name) {
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
 * @param command char* <command> that is parsed
 * @param name char* parsed name
 * @param id char* parsed id
 */
void parseCommandForNameAndId(char* command, char **name, char **id) {
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
 * @param command char* <command> that is parsed
 * @param name char* parsed name
 * @param id char* parsed id
 * @param content char* parsed content
 */
void parseCommandForNameAndIdAndContent(char* command, char **name, char **id, char** content) {
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
            if (numberOfSpaces < 4) {
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
 * @param command char* <command> that is parsed
 * @param name char* parsed name
 * @param content char* parsed content
 */
void parseCommandForNameAndContent(char* command, char** name, char** content) {
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
            if (numberOfSpaces < 3) {
                continue;
            }
        }
        if (numberOfSpaces >= 3) {
            // <id> part of command is located behind third space
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
 * Function concatenates name and id obtained from <command>.
 * In concatenation, '/' needs to inserted in between them.
 * The result is then stored in name parameter.
 *
 * @param name char* name obtained from <command>
 * @param id char* id obtained from <command>
 */
void concatNameAndId(char* name, char* id) {
    size_t two = 2;
    char* tmp = malloc(sizeof(char) * (strlen(name) + strlen(id) + two));
    if (tmp == NULL) {
        exit(EXIT_CODE_1);
    }

    strcpy(tmp, name);
    tmp[strlen(name)] = '/';
    strcat(tmp, id);

    name = realloc(name, sizeof(char) * (strlen(tmp) + 1));
    strcpy(name, tmp);
    free(tmp);
}

/**
 * Function builds HTTP request without body.
 * Request header contains method, board address(url), protocol version and hostname.
 *
 * @param httpRequest char** pointer to request that will be build
 * @param method char* HTTP request header method
 * @param url char* HTTP request header url
 * @param hostname char* HTTP request header hostname
 */
void createRequestWithoutBody(char** httpRequest, char* method, char* url, char* hostname) {
    unsigned long methodLen = strlen(method);
    unsigned long urlLen = strlen(url);
    unsigned long hostnameLen = strlen(hostname);
    char* host = (char*) malloc(sizeof(char) * (strlen("Host: ") + 1));
    strcpy(host, "Host: ");
    unsigned long hostLen = strlen(host);
    unsigned long requestSize = methodLen + urlLen + strlen(PROTOCOL_VERSION) + hostnameLen + hostLen + 8; // space, \r, \n, \r, \n, \r, \n, \0

    char* tmpRequest = (char*) malloc(sizeof(char) * (requestSize + 1));
    bzero(tmpRequest, requestSize);
    if (tmpRequest == NULL) {
        exit(EXIT_CODE_1);
    }

    strcpy(tmpRequest, method);
    strcat(tmpRequest, url);
    tmpRequest[methodLen + urlLen] = ' ';
    strcat(tmpRequest, PROTOCOL_VERSION);
    strcat(tmpRequest, "\r\n");
    strcat(tmpRequest, host);
    strcat(tmpRequest, hostname);
    strcat(tmpRequest, "\r\n\r\n");
    tmpRequest[requestSize] = '\0';

    *httpRequest = realloc(*httpRequest, sizeof(char) * (strlen(tmpRequest) + 1));
    strcpy(*httpRequest, tmpRequest);

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
 * @param hostname char* HTTP request header hostname
 * @param content char* body of HTTP request
 */
void createRequestWithBody(char** httpRequest, char* method, char* url, char* hostname, char* content) {
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
    unsigned long requestSize = methodLen + urlLen + strlen(PROTOCOL_VERSION) + hostnameLen + hostLen + contentTypeLen + strlen(contentLengthWithLengthOfContent) + contentLen + 12; // space, \r, \n, \r, \n, \r, \n, \r, \n, \r, \n, \0

    char* tmpRequest = (char*) malloc(sizeof(char) * (requestSize + 1));
    bzero(tmpRequest, requestSize);
    if (tmpRequest == NULL) {
        exit(EXIT_CODE_1);
    }

    strcpy(tmpRequest, method);
    strcat(tmpRequest, url);
    tmpRequest[methodLen + urlLen] = ' ';
    strcat(tmpRequest, PROTOCOL_VERSION);
    strcat(tmpRequest, "\r\n");
    strcat(tmpRequest, host);
    strcat(tmpRequest, hostname);
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
}

struct Api* newApi(char* command, char* apiEquivalent, int numberOfCommands) {
    struct Api* newApi = malloc(sizeof(struct Api));
    if (newApi == NULL) {
        return NULL;
    }

    newApi->commandArgument = malloc(sizeof(char) * (strlen(command) + 1));
    if (newApi->commandArgument == NULL) {
        return NULL;
    }
    strcpy(newApi->commandArgument, command);

    newApi->apiEquivalent = malloc(sizeof(char) * (strlen(apiEquivalent) + 1));
    if (newApi->apiEquivalent == NULL) {
        return NULL;
    }
    strcpy(newApi->apiEquivalent, apiEquivalent);

    newApi->numberOfCommands = numberOfCommands;
    return newApi;
}

void deleteApi(struct Api* api) {
    if (api != NULL) {
        free(api->apiEquivalent);
        free(api->commandArgument);
        free(api);
    }
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
void initiateCommunication(const int *clientSocket, struct sockaddr_in serverAddress, char** apiCommand, char** hostname){
    char* httpRequest = (char*) malloc(sizeof(char) * 1);
    httpRequest[0] = '\0';
    prepareHttpRequest(*apiCommand, *hostname, &httpRequest);

    //ssize_t r = 0;
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
    strcpy(request, httpRequest);

    // send request to server
    if ( write(*clientSocket, request, strlen(request)) < 0) {
        fprintf(stderr, "Writing to socket resulted in error.\n");
        close(*clientSocket);
        exit(1);
    }

    // if client was run with -n or -f argument
    /*if ( *l == 0) {
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

    }*/

    free(httpRequest);
    close(*clientSocket);
}