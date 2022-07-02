#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

/* Whether the client needs to use a different name */
bool isNameTaken = false;

/* Whether the initial WHO/NAME negotiation is complete */
bool handShakeComplete = false;

/* Stores the name of a client */
char clientName[NAME_SIZE];

/* Tracks the next number to append to name */
int nameCounter = 0;

void process_handshake_messages(ParsedMessage* parsedMessage,
        char* defaultName) {

    if (!strcmp(parsedMessage->command, "WHO")) {
        if (!isNameTaken) {
            printf("NAME:%s\n", defaultName);
            fflush(stdout);
            sprintf(clientName, "%s", defaultName);
        } else {
            printf("NAME:%s%d\n", defaultName, nameCounter);
            fflush(stdout);
            sprintf(clientName, "%s%d", defaultName, nameCounter); 
        }
    }

    if (!strcmp(parsedMessage->command, "NAME_TAKEN")) {
        if (isNameTaken) {
            nameCounter++;
        }
        isNameTaken = true;
    }

    if (!strcmp(parsedMessage->command, "YT")) {
            handShakeComplete = true;
    }

}

/**
 * Checks whether a command can be received before handshake is complete.
 *
 * Returns: True if it's a valid command false otherwise
 * */
bool is_valid_pre_handshake_command(char* command) {
    if (!strcmp(command, "MSG") 
            || !strcmp(command, "WHO")
            || !strcmp(command, "YT")
            || !strcmp(command, "NAME_TAKEN")
            || !strcmp(command, "KICK")) {
        return true;
    }

    return false;
}

/**
 * Returns whether the handshake is complete
 * */
bool handshake() {
    return handShakeComplete;
}

/**
 * Returns Gets the name of the client
 */
char* get_client_name() {
    return clientName;
}


