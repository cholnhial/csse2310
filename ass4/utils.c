#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdarg.h>

#include "utils.h"

LinkedList* linked_list_new(void) {
    LinkedList* list = (LinkedList*) malloc(sizeof(LinkedList));
    list->head = NULL;
    list->size = 0;
    pthread_mutex_init(&list->mutex, NULL);

    return list;
}

LinkedList* linked_list_add_item(LinkedList* list, void* item) {
   
    pthread_mutex_lock(&list->mutex);

    if (list->head == NULL) {
        list->head = (LinkedListNode*) malloc(sizeof(LinkedListNode));
        list->head->item = item;
        list->head->next = NULL;
        list->head->previous = NULL;
    } else {
        
        LinkedListNode* current = list->head;

        while (current->next != NULL) {
            current = current->next;
        }
        
        // Make the link 
        LinkedListNode* newNode = 
                (LinkedListNode*) malloc(sizeof(LinkedListNode));
        memset(newNode, 0, sizeof(LinkedListNode));
        newNode->item = item;
        newNode->next = current->next;
        
        current->next = newNode;
        newNode->previous = current;
    }
    list->size++;

    pthread_mutex_unlock(&list->mutex);

    return list;
}

LinkedList* linked_list_add_item_sorted(LinkedList* list, void* item,
        int (*compareCallback)(void* a, void* b)) {
    pthread_mutex_lock(&list->mutex);
    if (list->head == NULL) {
        list->head = (LinkedListNode*) malloc(sizeof(LinkedListNode));
        list->head->item = item;
        list->head->next = NULL;
        list->head->previous = NULL;
    } else if (compareCallback(list->head->item, item) >= 0) {
        // If the new node should be inserted at the beginning 
        // of the doubly linked list
        LinkedListNode* newNode = 
                (LinkedListNode*) malloc(sizeof(LinkedListNode));
        memset(newNode, 0, sizeof(LinkedListNode));
        newNode->item = item;
        newNode->next = list->head;
        newNode->next->previous = newNode;
        list->head = newNode;
    } else {
        LinkedListNode* current = list->head;
        // Locate the node where the new node is to be inserted in front of
        while (current->next != NULL && 
                compareCallback(current->next->item, item) < 0) {
            current = current->next;
        }
        // Make the link 
        LinkedListNode* newNode = 
                (LinkedListNode*) malloc(sizeof(LinkedListNode));
        memset(newNode, 0, sizeof(LinkedListNode));
        newNode->item = item;
        newNode->next = current->next;
        
        // If the new node was not inserted at the end 
        if (current->next != NULL) {
            newNode->next->previous = newNode;
        }
        
        current->next = newNode;
        newNode->previous = current;
    }

    list->size++;

    pthread_mutex_unlock(&list->mutex);

    return list;
}

LinkedList* linked_list_remove_item(LinkedList* list, void* item) {
   
    pthread_mutex_lock(&list->mutex);

    if (list->head != NULL && item != NULL) {
        LinkedListNode* current = list->head;
        
        // Find the element to be removed
        while (current->next != NULL && current->item != item) { 
            current = current->next;
        }

        if (current == NULL) {
            printf("It's NULL\n");
        }

        if (list->head == current) {
            list->head = current->next;
        }
        if (current->next != NULL) {
            current->next->previous = current->previous;
        }
        if (current->previous != NULL) {
            current->previous->next = current->next;
        }
        
        free(current);
        
        list->size--;
    }

    pthread_mutex_unlock(&list->mutex);
   
    return list;
}

void linked_list_print(LinkedList* list, FILE* stream, char* formatString) {
    LinkedListNode* current = list->head;

    while (current != NULL) {
        fprintf(stream, formatString, current->item);
        current = current->next;
    }
}

void linked_list_free(LinkedList* list) {
    if (list == NULL) { 
        return;
    };

    if (list->head != NULL) {
        LinkedListNode* current = list->head;
        while (current->next != NULL) {
            LinkedListNode* toBeFreed = current;
            current = current->next;
            free(toBeFreed);
            toBeFreed = NULL;
        }
    }
    free(list);
    list = NULL;
}

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
            if (line == NULL) {
                printf("Problem!\n");
                exit(0);
            }
        }
        fflush(file);
    }
   
    line[characterPosition] = 0;

    return line;
}

ParsedMessage* parse_message(char* message) {
    if (message == NULL) {
        return NULL;
    }
    /* To prevent message passed in from being modified */
    char* copy = strdup(message);
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
   
    char* found;

    while ((found = strsep(&copy, ":")) != NULL) {
        parsedMessage->arguments[parsedMessage->size] =
                (char*) malloc(strlen(found) + 1);
        strncpy(parsedMessage->arguments[parsedMessage->size],
                found, strlen(found));
        parsedMessage->arguments[parsedMessage->size][strlen(found)] = 0;

        int argsNewMemSize = parsedMessage->argsMemSize + sizeof(char*);
        parsedMessage->argsMemSize = argsNewMemSize;
        parsedMessage->arguments = (char**) realloc(parsedMessage->arguments,
                argsNewMemSize);
        parsedMessage->size++;
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
