#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/signal.h>

#include "common.h"
#include "utils.h"

/* A structure used to store each clients information and state */
typedef struct {
    char name[NAME_SIZE];
    FILE* childWrite;
    FILE* childRead;
    int pipeFromChild[2];
    int pipeToChild[2];
    pid_t pid;
    bool handShakeComplete;
    bool left;
} Client;

/* A structure used to track the clients talking with the server */
typedef struct {
    Client** clients;
    int memorySize;
    int size;
} ClientsList;

/* The list of clients that are talking with this server */
ClientsList* clientsList;

/* Somewhere to dump stderr for clients */
FILE* devNull;

/**
 * Initializes the clients list structure:
 *
 * Returns: a newly allocated ClientList, to be freed later when
 * done with
 * */
ClientsList* clients_list_init() {
    ClientsList* list = (ClientsList*) malloc(sizeof(ClientsList));
    list->clients = (Client**) malloc(sizeof(Client*));
    list->size = 0;
    list->memorySize = sizeof(Client*);
    return list;
}

/**
 * Adds a client to the client list structure.
 *
 * Parameters:
 *  list - the list to add the client to
 *  client - the client to be added
 *
 *  Returns: None
 * */
void clients_list_add(ClientsList* list, Client* client) {
    list->clients[list->size] = client;
    int newSize = list->memorySize + sizeof(Client*);
    list->memorySize = newSize;
    list->clients = (Client**) realloc(list->clients, newSize);
    list->size++;
}

/**
 * Frees memory that has bee allocated toa ClientList* 
 *
 * Parameters:
 *  list - the clients list to free
 *
 *  Returns: None
 *
 * */
void clients_list_free(ClientsList* list) {
    if (list->clients != NULL) {
        free(list->clients);
    }
    if (list != NULL) {
        free(list);
        list = NULL;
    }
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
        print_usage(stderr, "Usage: server configfile\n", EXIT_FAILURE);
    }
}

/**
 * Checks whether a line is a valid configuration line.
 *
 * Parameters:
 *  line - the line to check
 *
 * Returns true if the line is valid or false otherwise.
 * */
bool is_valid_config_line(char* line) {
    char* copy = (char*) malloc(strlen(line) + 1);
    strncpy(copy, line, strlen(line));
    copy[strlen(line)] = 0;

    if (strstr(copy, ":") == NULL) {
        free(copy);
        return false;
    }

    if (is_comment_line(line)) {
        free(copy);
        return false;
    }

    free(copy);

    return true;
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
LinesList* read_lines_from_config_file(char* filename) {

    FILE* file = fopen(filename, "r");

    LinesList* list = lines_list_init();

    char* line;
    while ((line = read_line(file, READ_MEM_ALLOCATION_CHUNK))) {
        if (is_valid_config_line(line)) {
            lines_list_add(list, line);
        }

    }
    return list;
}

/**
 * 
 * Launches the clients specified in the configuration.
 *
 * Parameters:
 *  configLines - the lines containing a client command and chat file
 *
 *  Returns: None
 * */
void launch_clients_from_config(LinesList* configLines) {
    for (int i = 0; i < configLines->size; i++) {
        ParsedMessage* config = parse_message(configLines->lines[i]);
        Client* client = (Client*) malloc(sizeof(Client));
        pipe(client->pipeToChild);
        pipe(client->pipeFromChild);
        client->handShakeComplete = false;    
        client->left = false;
        client->pid = fork();
        /* In the client if == 0 */
        if (client->pid == 0) {
            close(client->pipeToChild[1]); // Write end not needed
            close(client->pipeFromChild[0]); // Read end not needed

            dup2(client->pipeToChild[0], 0); // Read input from parent
            close(client->pipeToChild[0]); 
            dup2(client->pipeFromChild[1], 1); // Write output to parent
            close(client->pipeFromChild[1]);
            
            dup2(fileno(devNull), 2);
            if (execlp(config->command, config->command, 
                    config->arguments[1], NULL) == -1) {
                    client->left = true;
            }
        } else {
       
            close(client->pipeToChild[0]); 
            close(client->pipeFromChild[1]);

            client->childWrite = fdopen(client->pipeToChild[1], "w");
            client->childRead = fdopen(client->pipeFromChild[0], "r");
        }

        if(client->childWrite != NULL && client->childRead != NULL) {
            clients_list_add(clientsList, client);
        }
    }

}

/**
 * Checks whether all clients in the list have been all successfully
 * negotiated a name (HANDSHAKE).
 * 
 * 
 * Returns: True if negotiation for names is complete for all clients
 * false otherwise.
 * */
bool all_clients_handshakes_complete() {
    for (int i = 0; i < clientsList->size; i++) {
        if (!clientsList->clients[i]->handShakeComplete &&
                !clientsList->clients[i]->left) {
            return false;
        }
    }

    return true;
}

/**
 * Checks whether a client has already taken a name.
 *
 * Parameters:
 *  name - the name to check if it's taken
 *
 *
 *  Returns: True if the name is taken, false otherwise
 * */
bool is_name_taken(char* name) {
    for (int i = 0; i < clientsList->size; i++) {
        if (clientsList->clients[i]->handShakeComplete) {
            if (!strcmp(clientsList->clients[i]->name, name)) {
                return true;
            }
        }
    }

    return false;
}

/**
 *  Negotiates the name for a single client
 *
 *  Parameters:
 *      i - the client index in the client list
 *      parsedMessage - the parsed message from the client containing name
 *  
 *  Returns: True if name is taken false otherwise
 * */
bool negotiate_name(int i, ParsedMessage* parsedMessage) {
    bool name_is_taken = false;

    if (!is_name_taken(parsedMessage->arguments[1])) {
        strncpy(clientsList->clients[i]->name,
                parsedMessage->arguments[1], NAME_SIZE);
        
        int len = strlen(parsedMessage->arguments[1]); 
        clientsList->clients[i]->name[len] = 0; // terminate
                        
        clientsList->clients[i]->handShakeComplete = true;
        printf("(%s has entered the chat)\n", parsedMessage->arguments[1]);
        name_is_taken = false;
    } else {
        if (fprintf(clientsList->clients[i]->childWrite,
                "NAME_TAKEN:\n") == -1) {
            clientsList->clients[i]->left = true; // unreachable
        }
        fflush(clientsList->clients[i]->childWrite);
        name_is_taken = true;
    }
    
    return name_is_taken;
}

/**
 * Negotiates names with clients
 * 
 * Returns: None
 * */ 
void negotiate_names() {
    /* Negotiate Names */
    do {
        for (int i = 0; i < clientsList->size; i++) {
            if (clientsList->clients[i]->left || 
                    clientsList->clients[i]->handShakeComplete) { 
                continue;
            }

            bool name_is_taken = false;
            do {
                if (fprintf(clientsList->clients[i]->childWrite,
                            "WHO:\n") == -1) {
                    clientsList->clients[i]->left = true; // unreachable
                    break;
                }
                fflush(clientsList->clients[i]->childWrite);
                /* Read response */
                char* response = read_line(clientsList->clients[i]->childRead,
                        READ_MEM_ALLOCATION_CHUNK);    
                if (response == NULL) {
                    // client abruptly left
                    clientsList->clients[i]->left = true;
                } else {
                
                    ParsedMessage* parsedMessage = parse_message(response);
                    if (parsedMessage == NULL) {
                        continue;
                    }
                    name_is_taken = negotiate_name(i, parsedMessage); 
                    if(response != NULL) {
                        free(response);
                    }
                    free_parsed_message(parsedMessage);
                }
            } while(name_is_taken);
        }
    } while(!all_clients_handshakes_complete());
}

/**
 * Broadcast to all other active clients
 * another client has left
 *
 * */
void broadcast_leave(char* clientName) {
    
    for (int i = 0; i < clientsList->size; i++) {
        if (clientsList->clients[i]->left == false) {
            if (fprintf(clientsList->clients[i]->childWrite,
                    "LEFT:%s\n", clientName) == -1) {
                clientsList->clients[i]->left = true; // unreachable
            }
            fflush(clientsList->clients[i]->childWrite);
        }
    }
    printf("(%s has left the chat)\n", clientName);
}

/**
 * Broadcasts a message to all active clients
 * 
 * Paremeters:
 *  name - the client that sent the message
 *  message - the message to be broadcast
 *
 *  Returns: None
 * */
void broadcast_message(char* name, char* message) {
    for (int i = 0; i < clientsList->size; i++) {
        if (clientsList->clients[i]->left == false) {
            if (fprintf(clientsList->clients[i]->childWrite,
                    "MSG:%s:%s\n", name, message) == -1) {
                clientsList->clients[i]->left = true; // unreachable
            }
            fflush(clientsList->clients[i]->childWrite);
        }
    } 
}

/**
 * 
 * Kicks a client off on request
 * Client will no longer be communicated with.
 * 
 * Returns: None
 * */
void handle_kick(char* clientName) {
    for (int i = 0; i < clientsList->size; i++) {
        if (!clientsList->clients[i]->left && 
                !strcmp(clientsList->clients[i]->name, clientName)) {
            fprintf(clientsList->clients[i]->childWrite, "KICK:\n");
            fflush(clientsList->clients[i]->childWrite);
            clientsList->clients[i]->left = true;
        }
    }
}

/**
 * Checks if there's still clients around to talk to
 *
 * Returns: true if there's still clients to talk to
 * false otherwise
 * */
bool there_is_still_clients() {
    if (clientsList == NULL) {
        return false;
    }
    for (int i = 0; i < clientsList->size; i++) {
        if (!clientsList->clients[i]->left) {
            return true;
        }
    }

    return false;
}

/**
 * Signals whether a client is done sending or is quiting
 * 
 * Parameters:
 *  command - the command to check if its a QUIT or DONE
 *
 * Returns: True if client is done false otherwise
 * */
bool client_done(char* command) {
    if (!strcmp(command, "QUIT") || !strcmp(command, "DONE")) {
        return true;
    }

    return false;
}

/**
 *  The main messaing loop between server and clients.
 *  Will return when there's no longer clients left.
 *
 *  Returns: None
 * 
 * */
void messaging_loop() {
   
    do {
        for (int i = 0; i < clientsList->size; i++) {
         if (clientsList->clients[i]->left) {
                continue;
         }
          // Give the client a turn
          if (fprintf(clientsList->clients[i]->childWrite, "YT:\n") == -1) {
             clientsList->clients[i]->left = true;
             broadcast_leave(clientsList->clients[i]->name);
          }
          fflush(clientsList->clients[i]->childWrite);

          // Read response from client
          char* response = NULL;
          ParsedMessage* parsedMessage = NULL;

          do {
            response = read_line(clientsList->clients[i]->childRead,
                  READ_MEM_ALLOCATION_CHUNK);   
            if (response == NULL) {
                clientsList->clients[i]->left = true;
                broadcast_leave(clientsList->clients[i]->name);
                break;
            }
            parsedMessage = parse_message(response);

            if (!strcmp(parsedMessage->command, "CHAT")) {
                broadcast_message(clientsList->clients[i]->name, 
                        parsedMessage->arguments[1]);
                printf("(%s) %s\n", clientsList->clients[i]->name, 
                        parsedMessage->arguments[1]);
            }

            if (!strcmp(parsedMessage->command, "KICK")) {
                handle_kick(parsedMessage->arguments[1]);
            }

          } while(response != NULL && !client_done(parsedMessage->command));
        
            if (parsedMessage != NULL && !strcmp(parsedMessage->command,
                        "QUIT")) {
                clientsList->clients[i]->left = true;
                broadcast_leave(clientsList->clients[i]->name);
            }
         
        }

    } while (there_is_still_clients());
}

/** 
 *
 *  Where server starts doing work
 *
 *  Returns: None
 **/ 
void start() {

    if (!there_is_still_clients()) {
        return;
    }

    negotiate_names();
    messaging_loop();
}

int main(int argc, char** argv) {

    // Ignore SIGPIPE signal
    signal(SIGPIPE, SIG_IGN);

    LinesList* configLines;

    if (argc < 2) {
        print_usage(stderr, "Usage: server configfile\n", EXIT_FAILURE);
    }

    // To suppress client stderr
    devNull = fopen("/dev/null", "w");

    // Program will end if config file cannot be read
    exit_on_incorrect_file_access(argv[1]);
    
    configLines = read_lines_from_config_file(argv[1]);
    clientsList = clients_list_init();
    launch_clients_from_config(configLines);
    start();

    if (clientsList != NULL) {
        clients_list_free(clientsList);
    }
    
    if (configLines != NULL) {
        lines_list_free(configLines);
    }
    fclose(devNull);
    exit(EXIT_SUCCESS);
}
