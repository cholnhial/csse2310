#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <ctype.h>

#include "utils.h"

char* read_line(FILE* file, int readChunk) {
    if (file == NULL) {
        return NULL;
    }
    int chunkSize = readChunk;
    int characterPosition = 0;
    char* line = malloc(readChunk);
    int charRead;
    while ((charRead = fgetc(file)) != '\n') {
        if (charRead == EOF) {
            if (characterPosition > 0) {
                line[characterPosition] = 0;
                return line;
            }
            return NULL; // indicate we are done reading
        }
        line[characterPosition++] = charRead;   
        
        if (characterPosition == chunkSize) {
            chunkSize += readChunk;
            line = realloc(line, chunkSize);
        }
    }
   
    line[characterPosition] = 0;

    return line;
}

LinesList* lines_list_init() {
    LinesList* list = (LinesList*) malloc(sizeof(LinesList));
    list->lines = (char**) malloc(sizeof(char*));
    list->size = 0;
    list->memsize = sizeof(char*);

    return list;
}

void lines_list_add(LinesList* list, char* message) {
    list->lines[list->size] = message;
    int newSize = list->memsize + sizeof(char*);
    list->memsize = newSize;
    list->lines = (char**) realloc(list->lines, newSize);
    list->size++;
}

void lines_list_free(LinesList* list) {
    if (list->lines != NULL) {
        free(list->lines);
    }
    if (list != NULL) {
        free(list);
        list = NULL;
    }
}

void print_usage(FILE* file, char* message, int exitCode) {
    fprintf(file, message);

    exit(exitCode);
}

ParsedMessage* parse_message(char* message) {
    
    if (message == NULL) {
        return NULL;
    }
    /* To prevent message passed in from being modified */
    char* copy = (char*) malloc(strlen(message) + 1);
    strncpy(copy, message, strlen(message));
    copy[strlen(message)] = 0;

    if (strstr(copy, ":") == NULL) {
        free(copy);
        return NULL;
    }

    ParsedMessage* parsedMessage = 
            (ParsedMessage*) malloc(sizeof(ParsedMessage));
    parsedMessage->size = 0;
    parsedMessage->argsMemSize = sizeof(char*);
    parsedMessage->arguments = (char**) malloc(sizeof(char*));
    
    char* token = strtok(copy, ":");

    while (token != NULL) {
        parsedMessage->arguments[parsedMessage->size] =
                (char*) malloc(strlen(token) + 1);
        strncpy(parsedMessage->arguments[parsedMessage->size],
                token, strlen(token));
        parsedMessage->arguments[parsedMessage->size][strlen(token)] = 0;

        int argsNewMemSize = parsedMessage->argsMemSize + sizeof(char*);
        parsedMessage->argsMemSize = argsNewMemSize;
        parsedMessage->arguments = (char**) realloc(parsedMessage->arguments,
                argsNewMemSize);
        parsedMessage->size++;

        token = strtok(NULL, ":");
    }

    parsedMessage->command = parsedMessage->arguments[0];
    free(copy);
    return parsedMessage;
}

void free_parsed_message(ParsedMessage* parsedMessage) {
    if (parsedMessage->arguments != NULL) {
        free(parsedMessage->arguments);
    }
}

bool is_comment_line(char* line) {
    char* copy = (char*) malloc(strlen(line) + 1);
    strncpy(copy, line, strlen(line));
    copy[strlen(line)] = 0; // terminate string
    int i = 0;
    // Skip all the white space
    while (!isspace(copy[i]) && i < strlen(line)) {
        i++;
    }
    
    if (copy[i] == '#') {
        free(copy);
        return true;
    }
 
    free(copy);

    return false;
}
