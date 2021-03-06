#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>

#include "utils.h"
#include "common.h"
#include "netutils.h"


/* Contains information used by connecting thread */
typedef struct {
    char* authSecret;
    char* name;
    char* port;
    int serverSocket;
    FILE* socketWrite;
    FILE* socketRead;
} ServerConfig;

/* Holds the thread ids for the two client threads */
pthread_t threadIds[2];

/* The last command sent to client */
char lastCommand[COMMAND_SIZE];

/* Whether the client needs to use a different name */
bool isNameTaken = false;

/* Whether the initial WHO/NAME negotiation is complete */
bool handShakeComplete = false;

/* Whether the authentication is complete*/
bool authenticationComplete = false;

/* Stores the name of a client */
char clientName[NAME_SIZE];

/* Tracks the next number to append to name */
int nameCounter = 0;

/* A mutex used to prevent threads from concurrently modifying a variable */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Exits with authentication error code
 *
 * Returns: None
 * */
void terminate_on_authentication_error() {
    fprintf(stderr, "Authentication error\n");
    fflush(stderr);
    exit(EXIT_AUTHENTICATION_ERROR);
}

/** 
 *
 * Cleans up the sockets read and write ends
 *
 * Params:
 *  config - the server config info
 *
 *  Returns: NONE
 *
 **/
void close_socket(ServerConfig* config) {
    shutdown(config->serverSocket, SHUT_RDWR);
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
        print_usage(stderr, "Usage: client name authfile port\n",
                EXIT_FAILURE);

    }
}

/**
 * 
 * Performs the name negotiation between server
 *
 * Params:
 *  parsedMessage - the parsed message containing name negotiation command
 *  serverConfig - the server config info
 * 
 * Returns: None
 * */
void process_handshake_messages(ParsedMessage* parsedMessage, 
        ServerConfig* serverConfig) {

    if (!strcmp(parsedMessage->command, "WHO")) {
        if (!isNameTaken) {
            fprintf(serverConfig->socketWrite, "NAME:%s\n",
                    serverConfig->name);
            fflush(serverConfig->socketWrite);
            pthread_mutex_lock(&mutex);
            sprintf(clientName, "%s", serverConfig->name);
            pthread_mutex_unlock(&mutex);
        } else {
            fprintf(serverConfig->socketWrite, "NAME:%s%d\n",
                    serverConfig->name, nameCounter);
            fflush(serverConfig->socketWrite);
            pthread_mutex_lock(&mutex);
            sprintf(clientName, "%s%d", serverConfig->name, nameCounter); 
            pthread_mutex_unlock(&mutex);
        }
    }

    if (!strcmp(parsedMessage->command, "NAME_TAKEN")) {
        if (isNameTaken) {
            nameCounter++;
        }
        isNameTaken = true;
    }

    if (!strcmp(lastCommand, "WHO") && !strcmp(parsedMessage->command, "OK")) {
            handShakeComplete = true;
    }

}

/**
 *  
 *  Attempts to authenticate with server, if it fails the application stops
 *  Params:
 *      parsedMessage - the parsed command
 *      serverConfig - server configuration to reply back to
 *  Returns: None 
 **/
void authenticate_with_server(ParsedMessage* parsedMessage,
        ServerConfig* serverConfig) {

    if (!strcmp(parsedMessage->command, "AUTH")) {
        fprintf(serverConfig->socketWrite, "AUTH:%s\n",
                serverConfig->authSecret);
        fflush(serverConfig->socketWrite);
    }

    if (!strcmp(lastCommand, "AUTH") && 
            !strcmp(parsedMessage->command, "OK")) {
        authenticationComplete = true;
    }
}

/**
 * Checks whether a command can be received before handshake is complete.
 *
 * Returns: True if it's a valid command false otherwise
 * */
bool is_valid_pre_handshake_command(char* command) {
    if (!strcmp(command, "WHO")
            || !strcmp(command, "OK")
            || !strcmp(command, "AUTH")
            || !strcmp(command, "NAME_TAKEN")) {
        return true;
    }

    return false;
}

/**
 * Handles the MSG command from server
 *
 * Params:
 *  parsedMessage - the parsed protocol message
 *
 *  Returns: None
 *
 * */
void handle_msg_command(ParsedMessage* parsedMessage) {
    if (parsedMessage->size < 2) {
        terminate_on_communication_error();
    }

    char* name;
    char* message;
    if (parsedMessage->size == 2) {
        // no name specified
        name = parsedMessage->arguments[1];
        message = "";
    } else {
        name = parsedMessage->arguments[1];
        message = parsedMessage->arguments[2];
    }

    fprintf(stdout, "%s: %s\n", name, message);
    fflush(stdout);
}

/**
 *
 *  Processes the message recevied from the server
 *
 *  Parameters:
 *      parsedMessage - Contains the command and its arguments
 *      serverConfig - the server's config
 *
 *  Returns: None
 * */
void process_message(ParsedMessage* parsedMessage,
        ServerConfig* serverConfig) {

    if (!(authenticationComplete && handShakeComplete) &&
            !is_valid_pre_handshake_command(parsedMessage->command)) {
        terminate_on_communication_error(); 
    }

    if (!authenticationComplete && !handShakeComplete) {
        authenticate_with_server(parsedMessage, serverConfig);
    } else {
        // Negotiate a name
        process_handshake_messages(parsedMessage, serverConfig);
    }

    if (!strcmp(parsedMessage->command, "LIST") && handShakeComplete) {
        fprintf(stdout, "(current chatters: %s)\n", 
                parsedMessage->arguments[1]);
        fflush(stdout);
    }

    if (!strcmp(parsedMessage->command, "MSG")) {
        handle_msg_command(parsedMessage);
    }

    if (!strcmp(parsedMessage->command, "LEAVE")) {
        if (parsedMessage->size < 2) {
            terminate_on_communication_error();
        }

        fprintf(stdout, "(%s has left the chat)\n", 
                parsedMessage->arguments[1]);
        fflush(stdout);
    }

    if (!strcmp(parsedMessage->command, "ENTER")) {
        if (parsedMessage->size < 2) {
            terminate_on_communication_error();
        }
        fprintf(stdout, "(%s has entered the chat)\n", 
                parsedMessage->arguments[1]);
        fflush(stdout);
    }

    if (!strcmp(parsedMessage->command, "KICK")) {
        fprintf(stderr, "Kicked\n");
        fflush(stderr);
        exit(EXIT_KICKED);
    }
        
    strncpy(lastCommand, parsedMessage->command, COMMAND_SIZE - 1);
}

/**
 * A thread routine that handles communication between the client and server
 * Params:
 *  config - server config
 * Returns: thread exit status 
 * */
void* server_thread(void* config) {
    ServerConfig* serverConfig = (ServerConfig*) config; 
    int socketFileDescriptor = connect_to_tcp_server("localhost",
            serverConfig->port); 

    if (socketFileDescriptor == -1) {
        terminate_on_communication_error(); // other threads will be killed
    }

    pthread_mutex_lock(&mutex);
    int readFd = dup(socketFileDescriptor);
    serverConfig->socketWrite = fdopen(socketFileDescriptor, "w");
    serverConfig->socketRead = fdopen(readFd, "r");
    serverConfig->serverSocket = socketFileDescriptor;
    setvbuf(serverConfig->socketRead, NULL, _IONBF, 0);
    setvbuf(serverConfig->socketWrite, NULL, _IONBF, 0);
    pthread_mutex_unlock(&mutex);

    char* message;
    while (true) {
        if (feof(stdin)) {
            // We have no more input comming in from STDIN
            close_socket(serverConfig);
            fflush(stderr);
            pthread_exit((void*) EXIT_SUCCESS);
        }
        message = read_line(serverConfig->socketRead,
                READ_MEM_ALLOCATION_CHUNK);
        ParsedMessage* parsedMessage = parse_message(message);
        if (parsedMessage == NULL) {

            if (authenticationComplete) {
                terminate_on_communication_error();
            } else {
                terminate_on_authentication_error();           
            }
        }
        
        process_message(parsedMessage, serverConfig);
        free_parsed_message(parsedMessage);
        free(message);
    }
    pthread_exit((void*) 0);
}

/**
 * Checks whether the user's input starts with "*"
 *
 * Params:
 *  command - the command to check
 *
 * Returns: True if it's a command starting with *, 
 * false otherwise.
 *
 * */
bool is_server_command(char* command) {
    if (strstr(command, "*") != NULL && strstr(command, ":")) {
        return true;
    }

    return false;
}

/** 
 * 
 * Reads input continously from user/program
 *  config - the server's configuration
 *
 * Returns: thread exit status
 * 
 * */
void* input_thread(void* config) { 
    ServerConfig* serverConfig = (ServerConfig*) config;
    char* message = NULL;

    while (!handShakeComplete) {
        usleep(10000); // sleep for 10 miliseconds, don't eat CPU
    }
    
    int readFd = dup(STDIN_FILENO);
    FILE* stdinRead = fdopen(readFd, "r");

    while (!feof(stdinRead)) {
        message = read_line(stdinRead, READ_MEM_ALLOCATION_CHUNK);
        if (message == NULL) {
            break;
        }
        
        if (is_server_command(message)) {
            ParsedMessage* parsedMessage = parse_message(message + 1);
            if (parsedMessage->size >= 1 && 
                    !strcmp(parsedMessage->command, "LEAVE")) {
                close_socket(serverConfig);
                exit(EXIT_SUCCESS); // We will bring down the server thread too
            } else {
                fprintf(serverConfig->socketWrite, "%s\n", message + 1);
                fflush(serverConfig->socketWrite);
            }
            free(parsedMessage);
        } else if (handShakeComplete) {
            fprintf(serverConfig->socketWrite, "SAY:%s\n", message);
            fflush(serverConfig->socketWrite);
        }
        free(message);
    } 
    
    pthread_exit((void*) EXIT_SUCCESS);
}

/**
 *
 * Waits for the threads for input and server
 * * Returns: None
 * */ 
void wait_for_threads() {
    for (int i = 0; i < 2; i++) {
        pthread_join(threadIds[i], NULL);
    }
}

int main(int argc, char** argv) {
    
    if (argc < 3) {
        print_usage(stderr, "Usage: client name authfile port\n",
                EXIT_FAILURE);
    }

    // Program will end if authfile cannot be read
    exit_on_incorrect_file_access(argv[2]);
    
    char* authSecret = read_auth_secret(argv[2]); 
   
    ServerConfig serverConfig;
    serverConfig.authSecret = authSecret;
    serverConfig.name = argv[1];
    serverConfig.port = argv[3];
    
    // launch thread to read from stdin
    pthread_create(&threadIds[0], NULL, input_thread, &serverConfig);
    //launch thread to connect to server
    pthread_create(&threadIds[1], NULL, server_thread, &serverConfig);
    input_thread(&serverConfig);

    wait_for_threads();
    
    exit(EXIT_SUCCESS);
}
