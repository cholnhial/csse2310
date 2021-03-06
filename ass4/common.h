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

#define EXIT_AUTHENTICATION_ERROR 4

/* A structure that holds the parsed message */
typedef struct {
    char* command;
    char** arguments;
    int size;
    int argsMemSize;
} ParsedMessage;

/** 
 *
 * Exits with communication error code 
 *
 * Returns: None
 * */ 
void terminate_on_communication_error();

/**
 *
 *  Reads authentication secret from auth file
 *  
 *  Paramaters:
 *   filename - The file to read authentication info from
 *
 *   Returns: the authentication secret
 *
 *   Note: code adapted from assignment 3
 *
 * */
char* read_auth_secret(char* filename);


/**
 * Prints the applications usage message and exits with an
 * exit code specified.
 *
 * Params:
 * file - the stream to print the message to
 * message - the message to print to the screen
 * exitCode - the exit code to exit the program with
 *
 * Returns: None
 * */
void print_usage(FILE* file, char* message, int exitCode);

#endif
