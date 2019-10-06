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
//#include "api.h"
#include "../api.h"

int socketDescriptor;
struct Boards* boards;

// function checks and gets arguments
int checkArgs(int argc, char **argv);

// function creates pointer to structure holding all Boards
struct Boards* createBoards();

// function correctly disposes structure holding all boards
void deleteBoards();

// function recursively deletes all the boards
void deleteBoardsRecursively();

// function creates pointer to structure representing a Board
struct Board* createBoard(char* boardName);

// function correctly disposes structure representing a Board
void deleteBoard(struct Board* board);

// function recursively deletes all the boards from the current board
void deleteBoardRecursively(struct Board* board);

// function creates a pointer to structure holding board content
struct BoardContent* createBoardContent(int id, char* content);

// function recursively disposes structure holding board content
void deleteBoardContentRecursively(struct BoardContent* boardContent);

// function deletes board content without deleting other contents
void deleteBoardContent(struct BoardContent* boardContent);

struct HttpHeader* createHttpHeader();

// function correctly disposes HttpHeader structure
void deleteHttpHeader(struct HttpHeader* httpHeader);

// function tries to establish a server
void establishConnection(int portNumber, int *socketDescriptor, struct sockaddr_in* serverAddress);

// function communicates with client
void satisfyClient(const int* clientSocketDescriptor);

// function parses request
int parseRequest(char* request, struct HttpHeader *header, char** content);

// function parses header
int parseHeader(char* header, char** method, char** url);

// function process request by doing something with boards
int processRequest(struct HttpHeader *httpHeader, char *content, char **response);

// function parses board name from url
void parseBoardNameFromUrl(char* url, char** boardName, int boardNameLen, int offset);

// function parses board name and id from url
void parseBoardNameAndIdFromUrl(char *url, char **boardName, int *id, int boardNameAndIdLen, int offset);

// function fills response with existing boards
int apiGetBoards(char** response);

// function creates a board and adds it to the existing boards
int apiCreateNewBoard(char *boardName);

// function deletes a board and all it content from existing boards
int apiDeleteBoard(char* boardName);

// function fills response with a content of a board
int apiGetBoard(char* boardName, char** response);

// function fills response with a content of a board
int fillResponseWithBoardContent(struct BoardContent *boardContent, char **response);

// function puts content to a board
int apiPostToBoard(char *boardName, char *content, char **response);

// function deletes content id of a board
int apiDeleteContentFromBoard(char* boardName, int id, char** response);

// function puts content at proper of a board
int apiPutContentToBoard(char* boardName, int id, char* content, char** response);

// function checks whether a board is already created
int isBoardAlreadyCreated(char* boardName);

// function finds id of the last content of a board
void appendBoardContent(struct Board *board, char *content);

// function adds char to string
void addCharToString(char** stringToBeAddedTo, char addedChar);

// function deletes content at id
int deleteBoardContentForId(struct BoardContent *boardContent, int id, char **response);

// function over board content at id with new content
int putContentToBoardContent(struct BoardContent *boardContent, int id, char *content, char **response);

// function handles terminating signals
void sig_handler(int signo);

int main(int argc, char* argv[]) {
    int port, clientSocketDescriptor, pid;
    struct sockaddr_in serverAddress, clientAddress;
    boards = createBoards();
    if (boards == NULL) {
        fprintf(stderr, "Unable to create initial empty boards");
        exit(1);
    }

    // handle terminating signals
    // "
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        fprintf(stderr, "Cannot catch SIGINT\n");
        //exit(EXIT_CODE_1);
    }
    // " copied from: https://www.thegeekstuff.com/2012/03/catch-signals-sample-c-code

    // get arguments
    port = checkArgs(argc, argv);

    // make server
    establishConnection(port, &socketDescriptor, &serverAddress);
    // "
    socklen_t clientAddressLength = sizeof(clientAddress);
    // " inspired by: Rysavy Ondrej - DemoTcp/server.c

    int i = 0;
    while (i < 5) {
        // call to accept function
        // "
        clientSocketDescriptor = accept(socketDescriptor, (struct sockaddr *) &clientAddress, &clientAddressLength);
        // " inspired by: Rysavy Ondrej - DemoTcp/server.c

        // check if client socket descriptor is valid
        if (clientSocketDescriptor < 0) {
            fprintf(stderr, "Connecting to client resulted in error.\n");
            exit(EXIT_CODE_1);
        }

        satisfyClient(&clientSocketDescriptor);
        close(clientSocketDescriptor);  // close connection to client

        i++;
    }

    close(socketDescriptor);
    deleteBoardsRecursively();
    free(boards);
    exit(EXIT_CODE_0);
}

/**
 * Function handles terminating signals.
 * After such signal was caught, boards structure is deleted.
 *
 * @param signo int signal number
 */
 // "
void sig_handler(int signo) {
    if (signo == SIGINT) {
        fprintf(stderr, "received SIGINT");
    }

    close(socketDescriptor);
    deleteBoardsRecursively();
    free(boards);
    exit(EXIT_CODE_0);
}
// " copied from: https://www.thegeekstuff.com/2012/03/catch-signals-sample-c-code

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
                fprintf(stdout, "Server part for ISA project; try to run with \"-p portnumber\" arguments\ne.g. ./isaserver -p 1025\n");
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
 * Function creates structure that holds all Boards.
 *
 * @return pointer to structure Boards or NULL if malloc was not successful
 */
struct Boards* createBoards() {
    struct Boards* boards = malloc(sizeof(struct Boards));
    if (boards == NULL) {
        return NULL;
    }

    boards->board = NULL;

    return boards;
}

/**
 * Function deletes all the boards.
 *
 * @param boards structure holding pointer to first board
 */
void deleteBoards() {
    if (boards != NULL) {
        if (boards->board != NULL) {
            struct Board* nextBoard = boards->board;
            do {
                deleteBoard(boards->board);
                nextBoard = nextBoard->nextBoard;
            } while (nextBoard->nextBoard != NULL);
        }

        free(boards);
    }
}

void deleteBoardsRecursively() {
    if (boards != NULL) {
        if (boards->board != NULL) {
            deleteBoardRecursively(boards->board);
            free(boards->board);
        }
    }
}

/**
 * Function creates structure representing a board.
 *
 * @param boardName char* name of the new board
 *
 * @return pointer to structure Board or NULL if malloc was not successful
 */
struct Board* createBoard(char* boardName) {
    struct Board* board = malloc(sizeof(struct Board));
    if (board == NULL) {
        return NULL;
    }

    board->name = (char*) malloc(sizeof(char) * (strlen(boardName) + 1));
    strcpy(board->name, boardName);
    board->isInitialized = 1;
    board->content = NULL;
    board->nextBoard = NULL;

    return board;
}

/**
 * Function deletes a board with all its content.
 *
 * @param board struct Board* that will be deleted
 */
void deleteBoard(struct Board* board) {
    if (board != NULL) {
        free(board->name);
        deleteBoardContentRecursively(board->content);
        free(board->content);
        free(board);
    }
}

void deleteBoardRecursively(struct Board* board) {
    if (board != NULL) {
        deleteBoardRecursively(board->nextBoard);
        free(board->name);
        free(board->nextBoard);
        deleteBoardContentRecursively(board->content);
        free(board->content);
    }
}

/**
 * Function creates new structure holding content of the board.
 *
 * @param id int id of the content
 * @param content char* content
 *
 * @return pointe to structure BoardContent or NULL if malloc was not successful
 */
struct BoardContent* createBoardContent(int id, char* content) {
    struct BoardContent* boardContent = malloc(sizeof(struct BoardContent));
    if (boardContent == NULL) {
        return NULL;
    }

    boardContent->id = id;
    boardContent->content = (char*) malloc(sizeof(char) * (strlen(content) + 1));
    strcpy(boardContent->content, content);
    boardContent->nextContent = NULL;

    return boardContent;
}

/**
 * Function recursively deletes all BoardContent.
 *
 * @param boardContent struct BoardContent* that will be destroyed
 */
void deleteBoardContentRecursively(struct BoardContent* boardContent) {
    if (boardContent != NULL) {
        deleteBoardContentRecursively(boardContent->nextContent);
        free(boardContent->content);
        free(boardContent->nextContent);
    }
}

void deleteBoardContent(struct BoardContent* boardContent) {
    if (boardContent != NULL) {
        free(boardContent->content);
        //free(boardContent->nextContent) // todo really?
        free(boardContent);
    }
}

/**
 * Function creates structure that holds contents of HTTP header.
 *
 * @return pointer to structure HttpHeader or NULL if malloc was not successful
 */
struct HttpHeader* createHttpHeader() {
    struct HttpHeader* header = malloc(sizeof(struct HttpHeader));
    if (header == NULL) {
        return NULL;
    }

    header->method = (char*) malloc(sizeof(char) * 1);
    if (header->method == NULL) {
        header->method = NULL;
    } else {
        header->method[0] = '\0';
    }

    header->url = (char*) malloc(sizeof(char) * 1);
    if (header->url == NULL) {
        header->url = NULL;
    } else {
        header->url[0] = '\0';
    }

    header->host = (char*) malloc(sizeof(char) * 1);
    if (header->host == NULL) {
        header->host = NULL;
    } else {
        header->host[0] = '\0';
    }

    return header;
}

/**
 * Function disposes HttpHeader structure.
 * First if deallocates all memory used for method, url and host.
 *
 * @param httpHeader struct HttpHeader* that will be disposed
 */
void deleteHttpHeader(struct HttpHeader* httpHeader) {
    if (httpHeader != NULL) {
        free(httpHeader->method);
        free(httpHeader->url);
        free(httpHeader->host);
        free(httpHeader);
    }
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
 * @param boards struct Boards* pointer to all Boards
 */
void satisfyClient(const int* clientSocketDescriptor) {
    char request[BUFSIZ];
    char login[BUFSIZ];
    char response[BUFSIZ];
    char confirmationMessage[BUFSIZ];
    char tmp[BUFSIZ];
    bzero(request,BUFSIZ);
    bzero(login, BUFSIZ);
    bzero(response, BUFSIZ);
    bzero(confirmationMessage, BUFSIZ);
    bzero(tmp, BUFSIZ);
    char* header = (char*) malloc(sizeof(char) * 1);
    header[0] = '\0';
    char* content = (char*) malloc(sizeof(char) * 1);
    content[0] = '\0';
    int responseCode = RESPONSE_CODE_200;
    struct HttpHeader* httpHeader = createHttpHeader();
    char* httpResponse = (char*) malloc(sizeof(char) * 1);
    httpResponse[0] = '\0';

    // read request from client
    if ( read(*clientSocketDescriptor,request, BUFSIZ) < 0) {
        fprintf(stderr, "Reading from socket resulted in error.\n");
        close(*clientSocketDescriptor);
        exit(EXIT_CODE_1);
    }

    // parse request
    responseCode = parseRequest(request, httpHeader, &content);
    if (responseCode != RESPONSE_CODE_200) {
        // todo something went wrong during parsing
    }
    responseCode = processRequest(httpHeader, content, &httpResponse);
    if (responseCode != RESPONSE_CODE_200 || responseCode != RESPONSE_CODE_201) {
        // todo something went wrong during parsing
    }
    fprintf(stdout, "response code: %d\n", responseCode);
    fprintf(stdout, "response: %s\n", httpResponse);

    // check if communication is in set protocol
    /*if ( checkProtocol(request) != 0) {
        fprintf(stderr, "Communication is in unknown protocol.\n");
        close(*clientSocketDescriptor);
        exit(EXIT_CODE_1);
    }*/

    // find which option client wants
    /*size_t pos = strstr(request, "***") - request;
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
    endpwent();*/

    deleteHttpHeader(httpHeader);
    free(httpResponse);
    free(header);
    free(content);
}

/**
 * Function parses HTTP request.
 * First it parses header and prints it on stderr.
 * Then it parses content.
 *
 * @param request char* received request
 * @param header char** pointer to string of header that was obtained after parsing
 * @param content char** pointer to string of content that was obtained after parsing
 *
 * @return return code 200 if everything was OK otherwise 404 is returned of parsing error.
 */
int parseRequest(char* request, struct HttpHeader *header, char** content) {
    char* method = (char*) malloc(sizeof(char) * 1);
    method[0] = '\0';
    char* url = (char*) malloc(sizeof(char) * 1);
    url[0] = '\0';
    int parseResult = RESPONSE_CODE_200;

    // get header
    char* headerEnd = strstr(request, "\r\n\r\n");  // header and content is separated by blank line
    if (headerEnd == NULL) {
        fprintf(stderr, "Request is badly formatted");
        return RESPONSE_CODE_404;
    }

    int headerLen = strlen(request) - strlen(headerEnd);
    char* tmpHeader = (char*) malloc(sizeof(char) * (headerLen + 3)); // \r, \n, \0
    bzero(tmpHeader, headerLen + 3);
    for (int i = 0; i < headerLen; i++) {
        tmpHeader[i] = request[i];
    }
    tmpHeader[headerLen] = '\r';
    tmpHeader[headerLen+1] = '\n';
    tmpHeader[headerLen+2] = '\0';
    // print header to stderr
    fprintf(stderr, "Request header: %s\n", tmpHeader);
    parseResult = parseHeader(tmpHeader, &method, &url);

    header->method = realloc(header->method, sizeof(char) * (strlen(method) + 1));
    strcpy(header->method, method);
    header->url = realloc(header->url, sizeof(char) * (strlen(url) + 1));
    strcpy(header->url, url);

    // get content
    int contentLen = strlen(request) - strlen(tmpHeader) - 2;   // content is located behind blank line; \r\n
    if (contentLen > 0) {
        char* tmpContent = (char*) malloc(sizeof(char) * (contentLen + 1));
        bzero(tmpContent, contentLen+1);
        strncpy(tmpContent, request+strlen(tmpHeader)+2, contentLen);
        *content = realloc(*content, sizeof(char) * (strlen(tmpContent) + 1));
        strcpy(*content, tmpContent);
        free(tmpContent);
    }

    free(tmpHeader);
    free(method);
    free(url);

    return parseResult;
}

/**
 * Function parses header of HTTP request.
 *
 * @param header char* received HTTP header
 * @param method char** pointer to string specifying requested method
 * @param url char** pointer to string specifying requested board
 *
 * @return response code 200 if everything was OK, otherwise 404 is returned
 */
int parseHeader(char* header, char** method, char** url) {
    char* tmpMethod = (char*) malloc(sizeof(char) * 1);
    tmpMethod[0] = '\0';
    char* tmpUrl = (char*) malloc(sizeof(char) * 1);
    tmpUrl[0] = '\0';
    char* tmpProtocolVersion = (char*) malloc(sizeof(char) * 1);
    tmpProtocolVersion[0] = '\0';
    int numberOfSpaces = 0;
    int headerParsingResult = RESPONSE_CODE_200;

    for (int i = 0; i < strlen(header); i++) {
        if (header[i] == ' ' || (header[i] == '\r' && (i+1 < strlen(header)) && header[i+1] == '\n')) {
            numberOfSpaces++;
            continue;
        }

        if (numberOfSpaces == 0) {          // Method
            addCharToString(&tmpMethod, header[i]);
        } else if (numberOfSpaces == 1) {   // URL
            addCharToString(&tmpUrl, header[i]);
        } else if (numberOfSpaces == 2) {   // Protocol version
            addCharToString(&tmpProtocolVersion, header[i]);
        }
    }

    /*fprintf(stdout, "Method: %s\n", tmpMethod);
    fprintf(stdout, "Url: %s\n", tmpUrl);
    fprintf(stdout, "Protocol version: %s\n", tmpProtocolVersion);*/

    *method = realloc(*method, sizeof(char) * (strlen(tmpMethod) + 1));
    strcpy(*method, tmpMethod);
    *url = realloc(*url, sizeof(char) * (strlen(tmpUrl) + 1));
    strcpy(*url, tmpUrl);

    if (strcmp(tmpProtocolVersion, PROTOCOL_VERSION) != 0) {
        fprintf(stderr, "Bad protocol version; provided=%s\n required=%s\n", tmpProtocolVersion, PROTOCOL_VERSION);
        headerParsingResult = RESPONSE_CODE_404;
    }

    free(tmpMethod);
    free(tmpUrl);
    free(tmpProtocolVersion);

    return headerParsingResult;
}

int processRequest(struct HttpHeader *httpHeader, char *content, char **response) {
    int responseCode = RESPONSE_CODE_200;
    char* boardName = (char*) malloc(sizeof(char) * 1);
    boardName[0] = '\0';

    if (strstr(httpHeader->url, "/boards") != NULL) {    // request for boards
        if (strcmp(httpHeader->method, "GET") == 0) {           // GET /boards
            responseCode = apiGetBoards(response);
        } else if (strcmp(httpHeader->method, "POST") == 0) {   // POST /boards/<name>
            int boardNameLen = strlen(httpHeader->url) - strlen("/boards/");
            parseBoardNameFromUrl(httpHeader->url, &boardName, boardNameLen, strlen("/boards/"));
            responseCode = apiCreateNewBoard(boardName);
        } else if (strcmp(httpHeader->method, "DELETE") == 0) { // DELETE /boards/<name>
            int boardNameLen = strlen(httpHeader->url) - strlen("/boards/");
            parseBoardNameFromUrl(httpHeader->url, &boardName, boardNameLen, strlen("/boards/"));
            responseCode = apiDeleteBoard(boardName);
        } else {    // unknown HTTP method
            char* invalidMethod = "Unknown method for /boards manipulation";
            *response = realloc(*response, sizeof(char) * (strlen(invalidMethod) + 1));
            strcpy(*response, invalidMethod);
            responseCode = RESPONSE_CODE_404;
        }
    } else if (strstr(httpHeader->url, "/board")) {      // request for board
        if (strcmp(httpHeader->method, "GET") == 0) {           // GET /board/<name>
            int boardNameLen = strlen(httpHeader->url) - strlen("/board/");
            parseBoardNameFromUrl(httpHeader->url, &boardName, boardNameLen, strlen("/board/"));
            responseCode = apiGetBoard(boardName, response);
        } else if (strcmp(httpHeader->method, "POST") == 0) {   // POST /board/<name> <content>
            int boardNameLen = strlen(httpHeader->url) - strlen("/board/");
            parseBoardNameFromUrl(httpHeader->url, &boardName, boardNameLen, strlen("/board/"));
            responseCode = apiPostToBoard(boardName, content, response);
        } else if (strcmp(httpHeader->method, "DELETE") == 0) { // DELETE /board/<name>/<id>
            int boardNameAndIdLen = strlen(httpHeader->url) - strlen("/board/");
            int id = 0;
            parseBoardNameAndIdFromUrl(httpHeader->url, &boardName, &id, boardNameAndIdLen, strlen("/board/"));
            responseCode = apiDeleteContentFromBoard(boardName, id, response);
        } else if (strcmp(httpHeader->method, "PUT") == 0) {    // PUT /board/<name>/<id> <content>
            int boardNameAndIdLen = strlen(httpHeader->url) - strlen("/board/");
            int id = 0;
            parseBoardNameAndIdFromUrl(httpHeader->url, &boardName, &id, boardNameAndIdLen, strlen("/board/"));
            responseCode = apiPutContentToBoard(boardName, id, content, response);
        } else {    // unknown HTTP method
            char* invalidMethod = "Unknown method for /board manipulation";
            *response = realloc(*response, sizeof(char) * (strlen(invalidMethod) + 1));
            strcpy(*response, invalidMethod);
            responseCode = RESPONSE_CODE_404;
        }
    } else {    // url does not manipulate with boards
        char* invalidMethod = "Bad url";
        *response = realloc(*response, sizeof(char) * (strlen(invalidMethod) + 1));
        strcpy(*response, invalidMethod);
        responseCode = RESPONSE_CODE_404;
    }

    free(boardName);
    return responseCode;
}

/**
 * Function parses url to obtain board name. Board name is starting at offset position in url.
 * (e.g. for url /boards/name offset should be 7)
 *
 * @param url char* url from HTTP request header
 * @param boardName char** pointer to board name that will contain parsed name
 * @param boardNameLen int length of board name
 * @param offset int offset from which url prefix ends
 */
void parseBoardNameFromUrl(char* url, char** boardName, int boardNameLen, int offset) {
    char* tmpBoardName = (char*) malloc(sizeof(char) * (boardNameLen + 1));
    bzero(tmpBoardName, boardNameLen + 1);

    // copy from url starting on offset position
    strncpy(tmpBoardName, url+offset, boardNameLen);

    *boardName = realloc(*boardName, sizeof(char) * (strlen(tmpBoardName) + 1));
    strcpy(*boardName, tmpBoardName);

    free(tmpBoardName);
}

void parseBoardNameAndIdFromUrl(char *url, char **boardName, int *id, int boardNameAndIdLen, int offset) {
    parseBoardNameFromUrl(url, boardName, boardNameAndIdLen, offset);
    // *boardName contains "<name>/<id>

    char* idWithSlash = strchr(*boardName, '/');
    // copy id
    char* tmpId = (char*) malloc(sizeof(char) * (strlen(idWithSlash)));
    strncpy(tmpId, idWithSlash+1, strlen(idWithSlash)-1);
    // copy name
    int nameLen = strlen(idWithSlash) - strlen(tmpId);
    char* tmpName = (char*) malloc(sizeof(char) * nameLen);
    strncpy(tmpName, idWithSlash, nameLen-1);

    *boardName = realloc(*boardName, sizeof(char) * (strlen(tmpName) + 1));
    strcpy(*boardName, tmpName);
    long longId = strtol(tmpId, NULL, 10);
    *id = longId;

    free(tmpName);
    free(tmpId);
}

int apiGetBoards(char** response) {
    if (boards->board == NULL) {
        *response = realloc(*response, sizeof(char) * (strlen("No board exits") + 1));
        strcpy(*response, "No board exits");
        return RESPONSE_CODE_404;
    }

    struct Board* board = boards->board;
    char* tmpContent = (char*) malloc(sizeof(char) * 1);
    tmpContent[0] = '\0';
    char* tmp = (char*) malloc(sizeof(char) * 1);
    tmp[0] = '\0';

    while (board != NULL) {
        tmp = realloc(tmp, sizeof(char) * (strlen(tmpContent) + 1));
        strcpy(tmp, tmpContent);
        tmpContent = realloc(tmpContent, sizeof(char) * (strlen(tmp) + strlen(board->name) + 2));   // \n, \0
        strcpy(tmpContent, tmp);
        strcat(tmpContent, board->name);
        tmpContent[strlen(tmp)+strlen(board->name)] = '\n';
        tmpContent[strlen(tmp)+strlen(board->name)+1] = '\0';
        board = board->nextBoard;
    }

    *response = realloc(*response, sizeof(char) * (strlen(tmpContent) + 1));
    strcpy(*response, tmpContent);

    free(tmp);
    free(tmpContent);
    return RESPONSE_CODE_200;
}

/**
 * Function creates a new board and appends it to the list of already existing boards.
 * First it searches, whether a board already exits.
 *
 * @param boards structure Boards* holding pointer to the first board
 * @param boardName char* name of the new board
 *
 * @return 201 on success, 409 when board already exits, 404 if some other error occurred
 */
int apiCreateNewBoard(char* boardName) {
    if (isBoardAlreadyCreated(boardName) == 1) {
        return RESPONSE_CODE_409;
    }

    struct Board* newBoard = createBoard(boardName);
    if (newBoard == NULL) {
        return RESPONSE_CODE_404;
    }

    struct Board* first = boards->board;
    if (first == NULL) {
        boards->board = newBoard;
    } else {
        while (first->nextBoard != NULL) {
            first = first->nextBoard;
        }

        first->nextBoard = newBoard;
    }

    return RESPONSE_CODE_201;
}

/**
 * Function deletes a board with boardName from the list of boards.
 *
 * @param boards holding pointer to the first board
 * @param boardName char* name of the new board
 *
 * @return 200 if board was deleted, 404 if no such board exists or some other error occurred
 */
int apiDeleteBoard(char* boardName) {
    if (isBoardAlreadyCreated(boardName) == 0) {
        return RESPONSE_CODE_404;
    }

    struct Board* first = boards->board;
    if (first == NULL) {
        return RESPONSE_CODE_404;
    } else{
        if (strcmp(first->name, boardName) == 0) {  // board is first int the list
            struct Board* deletedBoard = first;
            boards->board = first->nextBoard;
            deleteBoard(deletedBoard);
            return RESPONSE_CODE_200;
        } else {    // board is not first in the list
            while (first->nextBoard != NULL) {
                if (strcmp(first->nextBoard->name, boardName) == 0) {
                    struct Board *deletedBoard = first->nextBoard;
                    first->nextBoard = deletedBoard->nextBoard; // shift all the boards
                    deletedBoard->nextBoard = NULL;
                    deleteBoard(deletedBoard);
                    return RESPONSE_CODE_200;
                } else {
                    first = first->nextBoard;
                }
            }
        }
    }

    return RESPONSE_CODE_404;
}

int apiGetBoard(char* boardName, char** response) {
    if (isBoardAlreadyCreated(boardName) == 0) {
        *response = realloc(*response, sizeof(char) * (strlen("No such board exits") + 1));
        strcpy(*response, "No such board exits");
        return RESPONSE_CODE_404;
    }

    struct Board* board = boards->board;
    do {
        if (strcmp(board->name, boardName) == 0) {
            if (board->content == NULL) {
                *response = realloc(*response, sizeof(char) * (strlen("No content for this board exits") + 1));
                strcpy(*response, "No content for this board exits");
                return RESPONSE_CODE_404;
            } else{
                return fillResponseWithBoardContent(board->content, response);
            }
        }
        board = board->nextBoard;
    } while (board != NULL);

    return RESPONSE_CODE_404;
}

int fillResponseWithBoardContent(struct BoardContent *boardContent, char **response) {
    char* tmpResponse = (char*) malloc(sizeof(char) * 1);
    tmpResponse[0] = '\0';
    char* tmp = (char*) malloc(sizeof(char) * 1);
    tmp[0] ='\0';
    struct BoardContent* tmpContent = boardContent;

    do {
        tmp = realloc(tmp, sizeof(char) * (strlen(tmpResponse) + 1));
        strcpy(tmp, tmpResponse);
        tmpResponse = realloc(tmpResponse, sizeof(char) * (strlen(tmp) + strlen(tmpContent->content) + 1));
        strcpy(tmpResponse, tmp);
        strcat(tmpResponse, tmpContent->content);

        tmpContent = tmpContent->nextContent;
    } while (tmpContent != NULL);

    *response = realloc(*response, sizeof(char) * (strlen(tmpResponse) + 1));
    strcpy(*response, tmpResponse);

    free(tmp);
    free(tmpResponse);
    return RESPONSE_CODE_200;
}

int apiPostToBoard(char *boardName, char *content, char **response) {
    if (isBoardAlreadyCreated(boardName) == 0) {
        *response = realloc(*response, sizeof(char) * (strlen("No such board exits") + 1));
        strcpy(*response, "No such board exits");
        return RESPONSE_CODE_404;
    }

    struct Board* board = boards->board;
    do {
        if (strcmp(board->name, boardName) == 0) {
            appendBoardContent(board, content);
            return RESPONSE_CODE_201;
        }
        board = board->nextBoard;
    } while (board != NULL);

    return RESPONSE_CODE_404;
}

int apiDeleteContentFromBoard(char* boardName, int id, char** response) {
    if (isBoardAlreadyCreated(boardName) == 0) {
        *response = realloc(*response, sizeof(char) * (strlen("No such board exits") + 1));
        strcpy(*response, "No such board exits");
        return RESPONSE_CODE_404;
    }

    struct Board* board = boards->board;
    do {
        if (strcmp(board->name, boardName) == 0) {
            return deleteBoardContentForId(board->content, id, response);
        }
        board = board->nextBoard;
    } while (board != NULL);

    return RESPONSE_CODE_404;
}

int apiPutContentToBoard(char* boardName, int id, char* content, char** response) {
    if (isBoardAlreadyCreated(boardName) == 0) {
        *response = realloc(*response, sizeof(char) * (strlen("No such board exits") + 1));
        strcpy(*response, "No such board exits");
        return RESPONSE_CODE_404;
    }

    struct Board* board = boards->board;
    do {
        if (strcmp(board->name, boardName) == 0) {
            return putContentToBoardContent(board->content, id, content, NULL);
        }
        board = board->nextBoard;
    } while (board != NULL);

    return RESPONSE_CODE_404;
}

/**
 * Function checks whether a board with boardName already exists.
 *
 * @param boards struct Boards* structure of all boards
 * @param boardName char* name of the board that is searched
 * @return 0 if no such board exists, 1 is board already exists
 */
int isBoardAlreadyCreated(char* boardName) {
    int alreadyCreated = 0;
    struct Board* board = boards->board;

    while (board != NULL && board->isInitialized == 1) {
        if (strcmp(board->name, boardName) == 0) {
            return 1;
        }
        board = board->nextBoard;
    }

    return alreadyCreated;
}

void appendBoardContent(struct Board *board, char *content) {
    if (board->content == NULL) {   // no content for current board, start with 1
        struct BoardContent* boardContent = createBoardContent(1, content);
        board->content = boardContent;
        return;
    }

    struct BoardContent* boardContent = board->content;
    while (boardContent->nextContent != NULL) {
        boardContent = boardContent->nextContent;
    }

    struct BoardContent* newBoardContent = createBoardContent(boardContent->id+1, content);
    boardContent->nextContent = newBoardContent;
}

int deleteBoardContentForId(struct BoardContent *boardContent, int id, char **response) {
    if (boardContent == NULL) {
        *response = realloc(*response, sizeof(char) * (strlen("No such board content exits") + 1));
        strcpy(*response, "No such board content exits");
        return RESPONSE_CODE_404;
    }

    struct BoardContent* first = boardContent;
    int foundToBeDeletedBoardContent = 0;
    while (first->nextContent != NULL) {
        if (foundToBeDeletedBoardContent == 1) {    // lower id for each content after deleted one
            first->id--;
        } else {
            if (first->id == id) {
                foundToBeDeletedBoardContent = 1;
                struct BoardContent* toBeDeletedBoardContent = first;
                first->nextContent = toBeDeletedBoardContent->nextContent;
                toBeDeletedBoardContent->nextContent = NULL;
                deleteBoardContent(toBeDeletedBoardContent);
            }
        }

        first = first->nextContent;
    }

    return RESPONSE_CODE_200;
}

int putContentToBoardContent(struct BoardContent *boardContent, int id, char *content, char **response) {
    if (boardContent == NULL) {
        *response = realloc(*response, sizeof(char) * (strlen("No such board content exits") + 1));
        strcpy(*response, "No such board content exits");
        return RESPONSE_CODE_404;
    }

    struct BoardContent* boardContent1 = boardContent;
    do {
        if (boardContent1->id == id) {
            boardContent1->content = realloc(boardContent1->content, sizeof(char) * (strlen(content) + 1));
            strcpy(boardContent1->content, content);
            return RESPONSE_CODE_200;
        }
        boardContent1 = boardContent1->nextContent;
    } while (boardContent1 != NULL);

    return RESPONSE_CODE_404;
}

/**
 * Function adds character to string. It reallocates string so that concatenation is possible.
 *
 * @param stringToBeAddedTo char** pointer to string that will be concatenated with new char
 * @param addedChar char that will be added to the string
 */
void addCharToString(char** stringToBeAddedTo, char addedChar) {
    char* tmp = (char*) malloc(sizeof(char) * (strlen(*stringToBeAddedTo) + 1));
    strcpy(tmp, *stringToBeAddedTo);
    tmp[strlen(*stringToBeAddedTo)] = '\0';

    char* tmp2 = (char*) malloc(sizeof(char) * (strlen(tmp) + 2));
    strcpy(tmp2, tmp);
    tmp2[strlen(tmp)] = addedChar;
    tmp2[strlen(tmp)+1] = '\0';
    free(tmp);

    *stringToBeAddedTo = realloc(*stringToBeAddedTo, sizeof(char) * (strlen(tmp2) + 1));
    strcpy(*stringToBeAddedTo, tmp2);
    free(tmp2);
}