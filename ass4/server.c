#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <sys/signal.h>

#include "utils.h"
#include "netutils.h"
#include "common.h"


/* Holds the information representing a single client */
typedef struct {
    char* name;
    int clientSocket;
    bool kicked;
    FILE* socketWrite;
    FILE* socketRead;
    int sayMessages;
    int listMessages;
    int kickMessages;
    
} Client;

/* Holds information for server stats to be displayed on SIGHUP */
typedef struct {
    int authMessages;
    int nameMessages;
    int sayMessages;
    int kickMessages;
    int listMessages;
    int leaveMessages;
    char* authSecret;
    bool noAuthentication;
} Server;

/* The messaging protocols we want to track stats */
enum MessageType {
    LIST, SAY, LEAVE, KICK, AUTH, NAME
};

/* Holds list of clients server is communicating with */
LinkedList* clientsList;

/* Holds sever info */
Server* server;

/* To prevent concurrent updates */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Represents signals to block */
static sigset_t signalMask;

/* Forward declarations */
void* serve_client(void* clientInfo);
void broadcast_leave(Client* leavingClient);

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
        print_usage(stderr, "Usage: server authfile [port]\n", EXIT_FAILURE);

    }
}

/**
 * A helper function to help with sorted insertion
 * into link list.
 *
 * see @strcmp
 *
 * Parameters:
 * clientA - the first client to compare
 * clientB - the second client to compare
 *
 * Returns: A value greater than or equal to zero when both clients
 * are have equal name or when ClientA has a lexigraphical order greater
 * than Client B returns values less than 0 when clientB has greater
 * order
 * */
int compare_client_names(void* clientA, void* clientB) {
    Client* firstClient = (Client*) clientA;
    Client* secondClient = (Client*) clientB;
    return strcmp(firstClient->name, secondClient->name);
}

/**
 * 
 *  A thread safe method to update server's stats
 *
 *  Parameters:
 *  type - the type of stat to update
 * 
 * Returns: None
 * */
void update_server_stat(enum MessageType type) {
    pthread_mutex_lock(&mutex);

    switch (type) {
        case SAY:
            server->sayMessages++;
            break;
        case LIST:
            server->listMessages++;
            break;
        case AUTH:
            server->authMessages++;
            break;
        case NAME:
            server->nameMessages++;
            break;
        case KICK:
            server->kickMessages++;
            break;
        case LEAVE:
            server->leaveMessages++;
            break;
    }

    pthread_mutex_unlock(&mutex);
}

/**
 * 
 *  Responsible for creating a thread to handle a client when called
 *  from tcp_server utility function.
 *
 *  Parameters:
 *  clientSocketFd - the socket which the client was accepted on
 *  
 *  Returns: None
 * */
void handle_client(int clientSocketFd) {
    int readFd = dup(clientSocketFd);
    FILE* socketWrite = fdopen(clientSocketFd, "w");
    FILE* socketRead = fdopen(readFd, "r");
    Client* client = (Client*) malloc(sizeof(Client));
    client->socketWrite = socketWrite;
    client->socketRead = socketRead;
    client->clientSocket = clientSocketFd;
    client->listMessages = 0;
    client->sayMessages = 0;
    client->kicked = false;
    client->kickMessages = 0;
    
    pthread_t tid;
    pthread_create(&tid, NULL, &serve_client, client);
}

/**
 *
 * Attempts to authenicate a client.
 * It will Immediately drop the client if authentication fails.
 *
 * Parameters:
 * client - the client to be authenticated
 *
 * Returns: True on success false otherwise
 * */
bool authenticate_client(Client* client) {
    fprintf(client->socketWrite, "AUTH:\n");
    fflush(client->socketWrite);
    char* message = read_line(client->socketRead,
            READ_MEM_ALLOCATION_CHUNK);
    ParsedMessage* parsedMessage = parse_message(message);
    if (parsedMessage == NULL || 
            !strcmp(parsedMessage->command, "AUTH") == false ||
            !strcmp(parsedMessage->arguments[1], "")) {
        return false;
    }

    if (!strcmp(parsedMessage->arguments[1], server->authSecret)) {
        update_server_stat(AUTH);
        fprintf(client->socketWrite, "OK:\n");
        fflush(client->socketWrite);
        free(message);
        free_parsed_message(parsedMessage);
        return true;
    }

    usleep(100000);
    return false;
}

/**
 * 
 * Checks whether there's a client already using a name.
 *
 * Parameters:
 *  name - the name to check
 *
 *
 *  Returns: True if taken false otherwise
 * */
bool is_name_taken(char* name) {
    for (LinkedListNode* current = clientsList->head;
            current != NULL; current = current->next) {
        if (!strcmp(((Client*) current->item)->name, name)) {
            return true;
        }
    }

    return false;
}

/**
 *  
 *  Performs the final part of the handshake by attempting to identify
 *  the client with a name.
 *
 *  Parameters:
 *  name - the client whose name is to be negotiated 
 *
 *  Returns: True if the client is granted a name false otherwise
 * */
bool negotiate_client_name(Client* client) {
    
    while (!feof(client->socketRead)) {
        fprintf(client->socketWrite, "WHO:\n");
        fflush(client->socketWrite);
        char* message = read_line(client->socketRead, 
                READ_MEM_ALLOCATION_CHUNK);
        ParsedMessage* parsedMessage = parse_message(message);
        if (parsedMessage == NULL || 
                !strcmp(parsedMessage->command, "NAME") == false ||
                parsedMessage->arguments[1] == NULL || 
                !strcmp(parsedMessage->arguments[1], "")) {
            return false;
        }
   
        if (!is_name_taken(parsedMessage->arguments[1])) {
            client->name = 
                    (char*) malloc(strlen(parsedMessage->arguments[1]) + 1);
            strncpy(client->name, parsedMessage->arguments[1],
                    strlen(parsedMessage->arguments[1]));
            client->name[strlen(parsedMessage->arguments[1])] = 0;
            fprintf(client->socketWrite, "OK:\n");
            fflush(client->socketWrite);
            free(parsedMessage);
            free(message);
            update_server_stat(NAME);
            return true;
        } else {
            update_server_stat(NAME);
            fprintf(client->socketWrite, "NAME_TAKEN:\n");
            fflush(client->socketWrite);
        }

        free_parsed_message(parsedMessage);
        free(message);
        usleep(100000);
    }

    return false;
}

/**
 * Executes a kick message from a client.
 * Send a KICK to the client
 * 
 * Parameters:
 *  parsedMessage - the parsed message with the client to kick etc
 *
 *  Returns: None
 * */
void execute_a_kick(ParsedMessage* parsedMessage) {
    for (LinkedListNode* current = clientsList->head;
            current != NULL; current = current->next) {
        Client* client = (Client*) current->item;
        if (!strcmp(client->name, parsedMessage->arguments[1]) &&
                client->socketWrite != NULL) {
            fprintf(client->socketWrite, "KICK:\n");
            fflush(client->socketWrite);
            client->kicked = true;
            broadcast_leave(client);
        }
    }
}

/**
 * Broadcasts a MSG command to all connected clients
 * from a specific client.
 *
 * Parameters:
 *  sender - the client sending the message
 *  message - the actual text message to broadcast
 *
 *  Return: None
 * */
void broadcast_message(Client* sender, char* message) {
    for (LinkedListNode* current = clientsList->head;
            current != NULL; current = current->next) {
        Client* client = (Client*) current->item;
        if (!feof(client->socketWrite)) {
            fprintf(client->socketWrite, "MSG:%s:%s\n", sender->name, message);
            fflush(client->socketWrite);
        }
    }
    
    fprintf(stdout, "%s: %s\n", sender->name, message);
    fflush(stdout);
}

/**
 * Broadcasts a ENTER message to all connected clients.
 * All clients are made aware of the new client.
 *
 * Parameters:
 *  newClient - the new client to announce
 *  
 *  Return: None
 * */
void broadcast_entry(Client* newClient) {
    for (LinkedListNode* current = clientsList->head;
            current != NULL; current = current->next) {
        Client* client = (Client*) current->item;
        fprintf(client->socketWrite, "ENTER:%s\n", newClient->name);
        fflush(client->socketWrite);
    }

    fprintf(stdout, "(%s has entered the chat)\n", newClient->name);
    fflush(stdout);
}

/**
 *
 * Broadcasts a LEAVE message to all connected clients
 * to announce a leaving client.
 *  
 * Parameters:
 *  leavingClient - the client to announce as leaving
 *
 * Return: None
 * */
void broadcast_leave(Client* leavingClient) {
    for (LinkedListNode* current = clientsList->head;
            current != NULL; current = current->next) {
        Client* client = (Client*) current->item;
        if (!strcmp(client->name, leavingClient->name) == false) {
            fprintf(client->socketWrite, "LEAVE:%s\n", leavingClient->name);
            fflush(client->socketWrite);
        }
    }

    fprintf(stdout, "(%s has left the chat)\n", leavingClient->name);
    fflush(stdout);
}

/**
 *
 * Builds the string containing comma seperated list of connected
 * client names.
 *
 * Returns: The comma seperated list of client names lexigraphical ordered
 * */
char* build_clients_list() {
    LinkedListNode* current = clientsList->head;
    char* list = NULL;

    if (current) {
        Client* client = (Client*) current->item;
        if (clientsList->size == 1) {
            return client->name;
        }
        int nextSize = strlen(client->name) + 1; // +1 for comma
        int chunkSize = nextSize;
        list = malloc(nextSize);
        strcpy(list, "");
        for (; current != NULL; current = current->next) {
            
            if (current->next != NULL) {
                sprintf(list, "%s%s,", list, client->name);
                LinkedListNode* next = current->next;
                client = (Client*) next->item;
                nextSize = strlen(client->name) + 1; // +1 for comma or \0
                chunkSize += nextSize;
                list = realloc(list, chunkSize);
            } else {
                sprintf(list, "%s%s", list, client->name);
            }
            
        }
    }

    return list;
}

/**
 *
 * Handles client messages after handshake (auth & name negotiation).
 *
 * Parameters:
 *  client - the client who sent the message
 *  parsedMessage - the actual message parsed
 *
 *  Return: None
 * */
void handle_client_message(Client* client,
        ParsedMessage* parsedMessage) {

    if (!strcmp(parsedMessage->command, "SAY")) {
        if (parsedMessage->arguments[1] != NULL &&
                !strcmp(parsedMessage->arguments[1], "") == false) {
            broadcast_message(client, parsedMessage->arguments[1]);
            update_server_stat(SAY);
            client->sayMessages++;
        }
    }

    if (!strcmp(parsedMessage->command, "KICK")) {
        if (parsedMessage->arguments[1] != NULL &&
                !strcmp(parsedMessage->arguments[1], "") == false) {
            execute_a_kick(parsedMessage);
            update_server_stat(KICK);
            client->kickMessages++;
        }
    }

    if (!strcmp(parsedMessage->command, "LIST")) {
        char* list = build_clients_list();
        fprintf(client->socketWrite, "LIST:%s\n", list);
        fflush(client->socketWrite);
        free(list);
        update_server_stat(LIST);
        client->listMessages++;
    }

    if (!strcmp(parsedMessage->command, "LEAVE")) {
        linked_list_remove_item(clientsList, client);
        shutdown(client->clientSocket, SHUT_RDWR);
        broadcast_leave(client);
        free(client->name);
        free(client);
        update_server_stat(LEAVE);
    }
}

/**
 * The main thread that serves a client
 * 
 * Parameters:
 *  clientInfo - the client being served
 *
 *  Returns: a interger
 * */
void* serve_client(void* clientInfo) {
    Client* client = (Client*) clientInfo;
    
    // Authenticate
    if (!server->noAuthentication) {
        if (!authenticate_client(client)) {
            shutdown(client->clientSocket, SHUT_RDWR);
            pthread_exit((void*) EXIT_FAILURE);
        }
    }

    // Perform name negotiation 
    if (negotiate_client_name(client)) {
        linked_list_add_item_sorted(clientsList, client,
                &compare_client_names);
        broadcast_entry(client);
    } else {
        // name negotiation failed/client disconnected
        shutdown(client->clientSocket, SHUT_RDWR);
        free(client);
        pthread_exit((void*) EXIT_FAILURE);
    }

    // Serve Loop
    while (!feof(client->socketWrite)) {
        char* message = read_line(client->socketRead,
                READ_MEM_ALLOCATION_CHUNK);
        ParsedMessage* parsedMessage = parse_message(message);
        if (parsedMessage == NULL) {
            break;
        }
        handle_client_message(client, parsedMessage);
        free(message);
        free_parsed_message(parsedMessage);
        usleep(100000); // rate limiting
    }

    // clean up client
    if (!client->kicked) {
        shutdown(client->clientSocket, SHUT_RDWR);
        linked_list_remove_item(clientsList, client);
        broadcast_leave(client);
        free(client->name);
        free(client);
    }

    pthread_exit((void*) EXIT_SUCCESS);
}

/**
 * Prints the server statistics to stderr
 *
 * Returns: None
 */
void print_stats() {
    
    fprintf(stderr, "@CLIENTS@\n");
    for (LinkedListNode* current = clientsList->head; 
            current != NULL; current = current->next) {
        Client* client = (Client*) current->item;
        fprintf(stderr, "%s:SAY:%d:KICK:%d:LIST:%d\n",
                client->name, client->sayMessages,
                client->kickMessages, client->listMessages);
    }

    fprintf(stderr, "@SERVER@\n");
    fprintf(stderr, "server:AUTH:%d:NAME:%d:SAY:%d:KICK:%d:LIST:%d:LEAVE:%d\n",
            server->authMessages, server->nameMessages, server->sayMessages,
            server->kickMessages, server->listMessages, server->leaveMessages);
    fflush(stderr);
}

/**
 * This thread is responsible for handling the SIGHUP signal
 * and print out server stats
 *
 * Inspired by post from pubs.opengroup.org
 *
 * Returns: Ignored, None
 * */
void* sighup_handling_thread(void* data) {
    int signalCaught;
    int ret;

    while (true) { 
        ret = sigwait(&signalMask, &signalCaught);
        if (ret != 0) {
            pthread_exit((void*) EXIT_FAILURE);
        }

        if (signalCaught == SIGHUP) {
            pthread_mutex_lock(&mutex);
            print_stats();
            pthread_mutex_unlock(&mutex);
        }
    }
}

int main(int argc, char** argv) {
    // Ignore SIGPIPE signal
    signal(SIGPIPE, SIG_IGN);

    if (argc < 2) {
        print_usage(stderr, "Usage: server authfile [port]\n", EXIT_FAILURE);
    }

    // Program will end if authfile cannot be read
    exit_on_incorrect_file_access(argv[1]);

    sigemptyset(&signalMask);
    sigaddset(&signalMask, SIGHUP);
    if (pthread_sigmask(SIG_BLOCK, &signalMask, NULL) != 0) {
        // deal with error
    } else {
        pthread_t sighupHandlerThread;
        pthread_create(&sighupHandlerThread, NULL, sighup_handling_thread,
                NULL);
    }
    clientsList = linked_list_new();
    server = (Server*) malloc(sizeof(Server));
    memset(server, 0, sizeof(Server));
    char* authSecret = read_auth_secret(argv[1]); 
    server->authSecret = authSecret;
    if (strlen(authSecret) == 0) {
        pthread_mutex_lock(&mutex);
        server->noAuthentication = true;
        pthread_mutex_unlock(&mutex);
    }
    int ret = tcp_server(argv[2] ? argv[2] : "-1", &handle_client,
            true, false);
    if (ret == -1) {
        terminate_on_communication_error();
    }

    exit(EXIT_SUCCESS);
}
