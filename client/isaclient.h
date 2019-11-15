//////////////////////////////////////////////////////////////
/*                                                          */
/*      Project for ISA course - HTTP bulletin board        */
/*      Header file for client part                         */
/*                                                          */
/*      Author: Marek Varga                                 */
/*      Login: xvarga14                                     */
/*                                                          */
//////////////////////////////////////////////////////////////
#ifndef PROJECT_ISACLIENT_H
#define PROJECT_ISACLIENT_H

char* hostname;
char* command;

// function checks and gets arguments
void getArgs(int argc, char* argv[], int* portNumber);

// function prepares http request header
void prepareHttpRequest(int portNumber, char **httpRequest);

// function parses <command> for name
void parseCommandForName(char **name);

// function parses <command> to name and id
void parseCommandForNameAndId(char **name, char **id);

// function parses <command> to name, id and content
void parseCommandForNameAndIdAndContent(char** name, char** id, char** content);

// function parses <command> to name and content
void parseCommandForNameAndContent(char** name, char** content);

// function creates HTTP request without body
void createRequestWithoutBody(char **httpRequest, char *method, char *url, int portNumber);

// function creates HTTP request with body
void createRequestWithBody(char **httpRequest, char *method, char *url, int portNumber, char *content);

// function finds server and creates socket
void findServer(const int *portNumber, struct hostent* server, struct sockaddr_in *serverAddress, int *clientSocket);

// function connects to server, sends requests and prints responses
void initiateCommunication(const int *clientSocket, struct sockaddr_in serverAddress, int portNumber);

// function parses HTTP request to header and content
int parseResponse(char* response, char** header, char** body);

// function parses HTTP response header
int parseHeader(char* request, int* responseCode, int* contentLength);

// function checks new line char
void checkNewLineChar(char** commandOption);

#endif //PROJECT_ISACLIENT_H
