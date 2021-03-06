#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "common.h"

/* Default client name */
#define DEFAULT_CLIENTBOT_NAME "clientbot"

/* The last command sent to client */
char lastCommand[COMMAND_SIZE];

/* Holds the lines the client will need to emit */
LinesList* responseList;

/* Holds the messages received so far */
LinesList* messagesReceived = NULL;

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
 * Checks whether a line from the reponse file is valid
 *
 * Parameters:
 *  line - the line to check
 *
 *  Returns: True if valid, false otherwise.
 *
 * */
bool is_valid_response(char* line) {
    char* copy = (char*) malloc(strlen(line) + 1);
    strncpy(copy, line, strlen(line));
    copy[strlen(line)] = 0; // terminate-string

    if (strstr(copy, ":") != NULL && !is_comment_line(line)) {
        free(copy);
        return true;
    }
    
    free(copy);
    return false;
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
        if (is_valid_response(line)) {
            lines_list_add(list, line);
        }
    }
    return list;
}

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
        print_usage(stderr, "Usage: clientbot responsefile\n", EXIT_FAILURE);

    }
}

/**
 * Checks whether a stimuli matches a message 
 * 
 * Parameters:
 *  stimuli - the stimuli to check
 *  message - the message to search for a match
 *
 *  Returns: True if a match is found, false otherwise
 * */
bool is_stimuli_a_match(char* stimuli, char* message) {
    if (strcasestr(message, stimuli) != NULL) {
        return true;
    }

    return false;
}

/**
 * Sends a response that matches the stimulus based on last 
 * message received
 *
 * Returns: None
 *
 * */
void send_response() {
    ParsedMessage* parsedResponse = NULL;
    for (int i = 0; i < messagesReceived->size; i++) {
        for (int j = 0; j < responseList->size; j++) {
            parsedResponse = parse_message(responseList->lines[j]);
            if (parsedResponse && 
                    is_stimuli_a_match(parsedResponse->arguments[0],
                    messagesReceived->lines[i])) {
                printf("CHAT:%s\n", parsedResponse->arguments[1]);
                fflush(stdout);
                break;
            }
        }
    }
    
    if (parsedResponse != NULL) {
        free_parsed_message(parsedResponse);
    }

    lines_list_free(messagesReceived);
    messagesReceived = NULL;
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
    if (!handshake()) {
        if (!is_valid_pre_handshake_command(parsedMessage->command)) {
            terminate_on_communication_error(); 
        }
        // Negotiate a name
        process_handshake_messages(parsedMessage, DEFAULT_CLIENTBOT_NAME);
    } 
    // Ready for general messages
    if (!strcmp(parsedMessage->command, "YT") && handshake()) {
        if (messagesReceived != NULL) { 
            send_response();
        }
        printf("DONE:\n");
        fflush(stdout);
    }
    if (!strcmp(parsedMessage->command, "MSG")) {
        if (parsedMessage->command == NULL ||
                parsedMessage->arguments[1] == NULL ||
                parsedMessage->arguments[2] == NULL ||
                parsedMessage->size < 3) {
            terminate_on_communication_error();
        }   
        fprintf(stderr, "(%s) %s\n", parsedMessage->arguments[1],
                parsedMessage->arguments[2]);
        if (messagesReceived == NULL) {
            messagesReceived = lines_list_init();
        }
        char* lastChatMessage =
                (char*) malloc(strlen(parsedMessage->arguments[2]) + 1);
        strncpy(lastChatMessage, parsedMessage->arguments[2],
                strlen(parsedMessage->arguments[2]));
        lastChatMessage[strlen(parsedMessage->arguments[2])] = 0;
        lines_list_add(messagesReceived, lastChatMessage); 
    }
    if (!strcmp(parsedMessage->command, "LEFT")) {
        if (parsedMessage->size < 2) {
            terminate_on_communication_error();
        }
        fprintf(stderr, "(%s has left the chat)\n", get_client_name());
    }

    if (!strcmp(parsedMessage->command, "KICK")) {
        lines_list_free(responseList);
        fprintf(stderr, "Kicked\n");
        exit(EXIT_KICKED);
    }
        
    strncpy(lastCommand, parsedMessage->command, COMMAND_SIZE - 1);
    lastCommand[strlen(parsedMessage->command)] = 0; // terminate string
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(stderr, "Usage: clientbot responsefile\n", EXIT_FAILURE);
    }

    // Program will end if response file cannot be read
    exit_on_incorrect_file_access(argv[1]);
    
    responseList = read_lines_from_file(argv[1]);
    
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
