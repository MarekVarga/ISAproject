#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>

int main(int argc, char* argv[]) {
    char* test = (char*) malloc(sizeof(char) * 5);
    bzero(test, 5);
    test = strcpy(test, "test");
    test[4] = '\0';
    char* temp = (char*) malloc(sizeof(char) * (strlen(test) + 1));
    bzero(temp, strlen(test) + 1);
    temp = strcpy(temp, test);
    temp[4] = '\0';
    //free(test);
    test = (char*) realloc(test, sizeof(char) * 9);
    bzero(test, 9);
    test = strcpy(test, temp);
    test[4] = '\0';
    test = strcat(test, temp);
    test[8] = '\0';
    free(test);
    test = NULL;
    int c = 1;
    free(temp);
}
