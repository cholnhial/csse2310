#ifndef COMMON_H_
#define COMMON_H_

#include <stdbool.h>

/* Default memory chunk allocation for read_line() */
#define READ_MEM_ALLOCATION_CHUNK 30

/* The size of a single command */
#define COMMAND_SIZE 32

/* The size of the name of a client */
#define NAME_SIZE 64

/* The exit code when the client is kicked */
#define EXIT_KICKED 3

/* The exit code when the client has a communication error */
#define EXIT_COMMUNICATION_ERROR 2

/* Whether the client needs to use a different name */
extern bool isNameTaken;

/* Whether the initial WHO/NAME negotiation is complete */
extern bool handShakeComplete;

/* Stores the name of a client */
char clientName[NAME_SIZE];

/* Tracks the next number to append to name */
extern int nameCounter;

/* A structure that holds lines */
typedef struct {
    char** lines;
    int size;
    int memsize;
} LinesList;

/* A structure that holds the parsed message */
typedef struct {
    char* command;
    char** arguments;
    int size;
    int argsMemSize;
} ParsedMessage;

/**
 * A sub process to process the WHO/NAME negotiation.
 *
 * Parameters: 
 *  parsedMessage - Contains the command and its arguments
 *  defaultName - the default name of the client
 *
 *  Returns: None
 * */
void process_handshake_messages(ParsedMessage* parsedMessage,
        char* defaultName);

/**
 * Checks whether a command can be received before handshake is complete.
 *
 * Returns: True if it's a valid command false otherwise
 * */
bool is_valid_pre_handshake_command(char* command);

/**
 * Returns whether the handshake is complete
 * */
bool handshake();

/**
 * Returns Gets the name of the client
 */
char* get_client_name();

#endif
