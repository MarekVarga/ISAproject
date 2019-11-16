//////////////////////////////////////////////////////////////
/*                                                          */
/*      Project for ISA course - HTTP bulletin board        */
/*      Header file for helper functions                    */
/*                                                          */
/*      Author: Marek Varga                                 */
/*      Login: xvarga14                                     */
/*                                                          */
//////////////////////////////////////////////////////////////
#ifndef PROJECT_UTILS_H
#define PROJECT_UTILS_H

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "api.h"

// function performs cleanup at exit, server and client have different implementation
void atExitFunction();

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

/**
 * Function converts integer to string.
 *
 * @param number int number to be converted
 * @param result pointer to char* result
 */
void intToString(int number, char** result) {
    // "
    char const digit[] = "0123456789";
    char* tmpResult = (char*) malloc(sizeof(char) * 1);
    tmpResult[0] = '\0';

    int i = number;
    if (number < 0) {
        tmpResult = realloc(tmpResult, sizeof(char) * 2);
        tmpResult[0] = '-';
        tmpResult[1] = '\0';
        i *= 1;
    }
    int numLen = 0;
    do {
        numLen++;
        i /= 10;
    } while (i);
    int isNegative = (int) strlen(tmpResult);
    tmpResult = realloc(tmpResult, sizeof(char) * (isNegative + numLen + 1));
    bzero(tmpResult, isNegative+numLen+1);
    do {
        tmpResult[--numLen] = digit[number%10];
        number /= 10;
    } while (number);
    // " inspired by https://stackoverflow.com/questions/9655202/how-to-convert-integer-to-string-in-c

    *result = realloc(*result, sizeof(char) * (strlen(tmpResult)+1));
    strcpy(*result, tmpResult);
    free(tmpResult);
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
 * Function concatenates name and id obtained from <command>.
 * In concatenation, '/' needs to inserted in between them.
 * The result is then stored in result parameter.
 *
 * @param name char* name obtained from <command>
 * @param id char* id obtained from <command>
 * @param result pointer to char* where result is stored
 */
void concatNameAndId(char* name, char* id, char** result) {
    char* tmp = malloc(sizeof(char) * (strlen(name) + strlen(id) + 2));
    if (tmp == NULL) {
        exit(EXIT_CODE_1);
    }

    strcpy(tmp, name);
    tmp[strlen(name)] = '/';
    tmp[strlen(name)+1] = '\0';
    strcat(tmp, id);
    tmp[strlen(name)+strlen(id)+1] = '\0';

    *result = realloc(*result, sizeof(char) * (strlen(tmp) + 1));
    strcpy(*result, tmp);
    free(tmp);
}

/**
 * Function checks whether given board name contains only alpha numeric characters.
 *
 * @param name const char* checked board name
 *
 * @return boolean 1 if board name contains only alpha numeric chars otherwise 0 is returned
 */
int isBoarNameAlphaNumeric(const char* name) {
    for (int i = 0; i < (int) strlen(name); ++i) {
        if (!isalnum(name[i])) {
            return 0;
        }
    }

    return 1;
}

#endif //PROJECT_UTILS_H
