//
// Created by marek on 10/2/19.
//

#ifndef PROJECT_API_H
#define PROJECT_API_H

char* PROTOCOL_VERSION = "HTTP/1.1";
char* CONTENT_TYPE = "Content-Type: text/plain";
char* CONTENT_LENGTH = "Content-Length: ";

struct Boards {
    struct Board* board;
};

struct Board {
    char* name;
    int isInitialized;
    struct BoardContent* content;
    struct Board* nextBoard;
};

struct BoardContent {
    int id;
    char* content;
    struct BoardContent* nextContent;
};

struct HttpHeader {
    char* method;
    char* url;
    char* host;
};

char* GET_BOARDS = "GET /boards";
char* POST_BOARDS = "POST /boards/";
char* DELETE_BOARDS = "DELETE /boards/";
char* GET_BOARD = "GET /board/";
char* POST_BOARD = "POST /board/";
char* DELETE_BOARD = "DELETE /board/";
char* PUT_BOARD = "PUT /board/";

char* RESPONSE_200 = "HTTP/1.1 200 OK\r\n";
char* RESPONSE_201 = "HTTP/1.1 201 CREATED\r\n";
char* RESPONSE_400 = "HTTP/1.1 400 BAD REQUEST\r\n";
char* RESPONSE_404 = "HTTP/1.1 404 NOT FOUND\r\n";
char* RESPONSE_409 = "HTTP/1.1 409 CONFLICT\r\n";

#endif //PROJECT_API_H
