//
// Created by marek on 10/2/19.
//

#ifndef PROJECT_ISACLIENT_H
#define PROJECT_ISACLIENT_H

int EXIT_CODE_0 = 0;
int EXIT_CODE_1 = 1;

typedef struct ApiServiceContent{
    char* commandArgument;
    int numberOfCommands;
    char* apiEquivalent;
} ApiService;

char* GET_BOARDS = "GET /boards";
char* POST_BOARDS = "POST /boards/";
char* DELETE_BOARDS = "DELETE /boards/";
char* GET_BOARD = "GET_/board/";
char* POST_BOARD = "POST /board/";
char* DELETE_BOARD = "DELETE /board/";
char* PUT_BOARD = "PUT /board/";



#endif //PROJECT_ISACLIENT_H
