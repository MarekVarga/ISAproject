//////////////////////////////////////////////////////////////
/*                                                          */
/*      Project for ISA course - HTTP bulletin board        */
/*      Header file for server part                         */
/*                                                          */
/*      Author: Marek Varga                                 */
/*      Login: xvarga14                                     */
/*                                                          */
//////////////////////////////////////////////////////////////
#ifndef PROJECT_ISASERVER_H
#define PROJECT_ISASERVER_H

int RESPONSE_CODE_200 = 200;
int RESPONSE_CODE_201 = 201;
int RESPONSE_CODE_400 = 400;
int RESPONSE_CODE_404 = 404;
int RESPONSE_CODE_409 = 409;

int socketDescriptor;
struct Boards* boards;

// function checks and gets arguments
int checkArgs(int argc, char **argv);

// function creates pointer to structure holding all Boards
void createBoards();

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
void establishConnection(int portNumber, struct sockaddr_in* serverAddress);

// function communicates with client
void satisfyClient(const int* clientSocketDescriptor);

// function parses request
int parseRequest(char *request, struct HttpHeader *header, char **content, char **errorResponse);

// function parses header
int parseHeader(char* header, char** method, char** url, int* contentLength, char** errorResponse);

// function process request by doing something with boards
int processRequest(struct HttpHeader *httpHeader, char *content, char **response);

// function parses board name from url
void parseBoardNameFromUrl(char* url, char** boardName, int boardNameLen, int offset);

// function parses board name and id from url
int parseBoardNameAndIdFromUrl(char *url, char **boardName, int *id, int boardNameAndIdLen, int offset);

// function fills response with existing boards
int apiGetBoards(char** response);

// function creates a board and adds it to the existing boards
int apiCreateNewBoard(char *boardName, char **response);

// function deletes a board and all it content from existing boards
int apiDeleteBoard(char *boardName, char **response);

// function fills response with a content of a board
int apiGetBoard(char* boardName, char** response);

// function fills response with a content of a board
int fillResponseWithBoardContent(struct BoardContent *boardContent, char *boardName, char **response);

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

// function deletes content at id
int deleteBoardContentForId(struct BoardContent **boardContent, int id, char **response);

// function over board content at id with new content
int putContentToBoardContent(struct BoardContent *boardContent, int id, char *content, char **response);

// function prepares HTTP response
void prepareHttpResponse(int responseCode, char *serverResponse, char **httpResponse);

// function prepares HTTP respnse header
void constructHttpHeader(char* statusLine, int contentLength, char** httpHeader);

// function handles terminating signals
void signalHandler(int signalNumber);

#endif //PROJECT_ISASERVER_H
