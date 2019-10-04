//
// Created by marek on 10/2/19.
//

#ifndef PROJECT_API_H
#define PROJECT_API_H

char* PROTOCOL_VERSION = "HTTP/1.1";
char* CONTENT_TYPE = "Content-Type:text/plain";
char* CONTENT_LENGTH = "Content-Length:";

typedef struct ApiServiceContent{
    char* commandArgument;
    int numberOfCommands;
    char* apiEquivalent;
} ApiService;

struct Api {
    char* commandArgument;
    char* apiEquivalent;
    int numberOfCommands;
};

char* GET_BOARDS = "GET /boards";
char* POST_BOARDS = "POST /boards/";
char* DELETE_BOARDS = "DELETE /boards/";
char* GET_BOARD = "GET_/board/";
char* POST_BOARD = "POST /board/";
char* DELETE_BOARD = "DELETE /board/";
char* PUT_BOARD = "PUT /board/";

#endif //PROJECT_API_H
