#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "common.h"

/* Default client name */
#define DEFAULT_CLIENT_NAME "client"

/* The last command sent to client */
char lastCommand[COMMAND_SIZE];

/* Points to the next line the client needs to send */
int nextLine = 0;

/* Holds the lines the client will need to emit */
LinesList* linesList;

/* Whether all lines in chat YT have been sent */
bool chatDone = false;

/**
 * 
 * Checks whether the file exists or can be read.
 * Exits with an error code indicating failure.
 *
 * Returns nothing
 * 
 * Note: code from assignment 1
 * */
void exit_on_incorrect_file_access(char* filename) {

    /* Check if file exists and if we can read */
    int returnValue = access(filename, F_OK | R_OK);

    if (returnValue == -1) {
        print_usage(stderr, "Usage: client chatscript\n", EXIT_FAILURE);

    }
}

/**
 *
 *  Reads all the lines from a file and puts them into the structure.
 *  The structure has the lines and how many there is for easy iteration.
 *  
 *  Paramaters:
 *   filename - The file to read the lines from
 *
 *   Returns the struct containing the lines read
 *
 *   Note: code adapted from assignment 1
 *
 * */
LinesList* read_lines_from_file(char* filename) {

    FILE* file = fopen(filename, "r");

    LinesList* list = lines_list_init();

    char* line;
    while ((line = read_line(file, READ_MEM_ALLOCATION_CHUNK))) {
        lines_list_add(list, line);
    }
    return list;
}

/**
 * 
 * Validate a chatscript line
 * 
 * Parameters:
 *  line - the line to validate
 * Returns: True if valid false otherwise
 *
 * */
bool is_invalid_chatscript_line(char* line) {
    char* copy = (char*) malloc(strlen(line) + 1);
    strncpy(copy, line, strlen(line));
 
    if (strstr(copy, ":") == NULL) {
        free(copy);
        return true;
    }
    free(copy);

    char* lineToParse = (char*) malloc(strlen(line) + 1);
    strncpy(lineToParse, line, strlen(line));
    ParsedMessage* parsedMessage = parse_message(lineToParse);

    if (!strcmp(parsedMessage->command, "CHAT")) {
        free(lineToParse);
        free(parsedMessage);
        return false;
    } else if (!strcmp(parsedMessage->command, "DONE")) {
        free(parsedMessage);
        free(lineToParse);
        return false;
    } else if (!strcmp(parsedMessage->command, "QUIT")) {
        free(parsedMessage);
        free(lineToParse);
        return false;
    } else {
        free(lineToParse);
        free(parsedMessage);
        return true;
    }

}

/**
 * Sends the next line to the server.
 *
 * Returns: None
 * */
void send_next_line() {
    
    if (nextLine < linesList->size && !chatDone) {
        
        // Skip all bad lines until we reach a good line
        while (is_invalid_chatscript_line(linesList->lines[nextLine])) {
            nextLine++;
        }

        char* line = (char*) malloc(strlen(linesList->lines[nextLine]) + 1);
        strncpy(line, linesList->lines[nextLine], 
                strlen(linesList->lines[nextLine]));
        ParsedMessage* parsedMessage = parse_message(line);
 
        printf("%s\n", linesList->lines[nextLine]);
        fflush(stdout);

        if (!strcmp(parsedMessage->command, "QUIT")) {
            lines_list_free(linesList);
            free(line);
            exit(EXIT_SUCCESS);
        }

        if (!strcmp(parsedMessage->command, "DONE")) {
            chatDone = true;
        }
        
        free(line);
        free(parsedMessage);
        nextLine++;
    } else {
        chatDone = true;
    }
}

/** 
 *
 * Exits with communication error code 
 *
 * Returns: None
 * */
void terminate_on_communication_error() {
    fprintf(stderr, "Communications error\n");
    exit(EXIT_COMMUNICATION_ERROR);
}

/**
 *
 *  Processes the message recevied from the server
 *
 *  Parameters:
 *      parsedMessage - Contains the command and its arguments
 *
 *  Returns: None
 * */
void process_message(ParsedMessage* parsedMessage) {

    if (!handShakeComplete) {
        if (!is_valid_pre_handshake_command(parsedMessage->command)) {
            terminate_on_communication_error(); 
        }
        // Negotiate a name
        process_handshake_messages(parsedMessage, DEFAULT_CLIENT_NAME);
    } 

    // Ready for general messages
    if (!strcmp(parsedMessage->command, "YT") && handShakeComplete) {
        chatDone = false;
        do {
            send_next_line();
        } while(!chatDone);
    }

    if (!strcmp(parsedMessage->command, "MSG")) {
        if (parsedMessage->arguments[2] == NULL || parsedMessage->size < 3) {
            terminate_on_communication_error();
        }
        fprintf(stderr, "(%s) %s\n", parsedMessage->arguments[1],
                parsedMessage->arguments[2]);
    }

    if (!strcmp(parsedMessage->command, "LEFT")) {
        if (parsedMessage->size < 2) {
            terminate_on_communication_error();
        }
        fprintf(stderr, "(%s has left the chat)\n", get_client_name());
    }

    if (!strcmp(parsedMessage->command, "KICK")) {
        lines_list_free(linesList);
        fprintf(stderr, "Kicked\n");
        exit(EXIT_KICKED);
    }
        
    strncpy(lastCommand, parsedMessage->command, COMMAND_SIZE - 1);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(stderr, "Usage: client chatscript\n", EXIT_FAILURE);
    }

    // Program will end if chatscript cannot be read
    exit_on_incorrect_file_access(argv[1]);
    
    linesList = read_lines_from_file(argv[1]);

    char* message;

    while (true) {
        message = read_line(stdin, READ_MEM_ALLOCATION_CHUNK);
        ParsedMessage* parsedMessage = parse_message(message);
        if (parsedMessage == NULL) {
            terminate_on_communication_error();
        }
        process_message(parsedMessage);
        free_parsed_message(parsedMessage);
        free(message);
    }
    
    exit(EXIT_SUCCESS);
}
