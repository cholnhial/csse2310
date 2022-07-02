#ifndef UTILS_H
#define UTILS_H_

#include <pthread.h>

#include "common.h"

/****
 *
 * Some of the utility methods here are adapted from assignment 1
 *
 * */

/* Represents a single node of the linked list */
typedef struct LinkedListNode LinkedListNode;
struct LinkedListNode {
    void* item;
    LinkedListNode* next;
    LinkedListNode* previous;
};

/* Represents the linked list with head and size */
typedef struct {
    LinkedListNode* head;
    int size;
    pthread_mutex_t mutex;
} LinkedList;

/**
 * 
 * Reads a line from a file 
 * making sure to allocate enough memory to store it.
 * 
 * Parameters:
 *  file - the file to read from
 *  readChunk - the chunk size of memory to allocate at a time
 *
 *  Returns: The line read and allocated. 
 *  Line should be freed when no longer needed
 *
 *  NOTE: reuse from assignment 3
 **/
char* read_line(FILE *file, int readChunk);

/**
 * Prints the programs usage. 
 * 
 * Parameters:
 *  file - The file to print to
 *  message - The message to print
 *  exitCode - The exit code number to exit the program with
 * 
 *  NOTE: reuse from assignment 3
 *  Returns: None
 * */
void print_usage(FILE* file, char* message,  int exitCode);

/**
 *
 * Parses a message by splitting it into the command part and payload
 * which is then stored in a struct (ParsedMessage).
 * 
 * Parameters:
 *  message - The message to parse
 * 
 * NOTE: reuse from assignment 3
 *
 * Returns: The parsed message in a struct pointer which must be freed
 * after use.
 * */
ParsedMessage* parse_message(char* message);

/**
 * Frees memory allocated to a ParsedMessage struct.
 *
 * Parameters:
 *  parsedMessage* - the ParsedMesage pointer to have resources freed
 *
 *  NOTE: reuse from assignment 3
 * Returns: None
 * */
void free_parsed_message(ParsedMessage* parsedMessage);


/**
 *
 * Creates a new empty linked list 
 * 
 * Returns: A new linked list to be responsibly freed later
 **/
LinkedList* linked_list_new(void);

/**
 *   Adds a new item to the list
 *
 *   Parameters:
 *      list - the linked list to add the new item to
 *      item - the item to be added
 *
 *      
 *  Returns: A pointer to the list that was just added to
 * */
LinkedList* linked_list_add_item(LinkedList* list, void* item);

/**
 *   A Generic sorted insertion that uses a compare function provided 
 *   by user
 *
 *   Parameters:
 *      list - the linked list to add the new item to
 *      item - the item to add in sorted fashion
 *  
 *  NOTES: used https://www.geeksforgeeks.org/insert-value-sorted-way-sorted-doubly-linked-list/ as a reference
 * 
 *  Returns: A pointer to the list that was just added to
 * */
LinkedList* linked_list_add_item_sorted(LinkedList* list, void* item, 
        int (*compareCallback)(void *a, void* b));

/**
 * Removes an item from the list
 *
 * Parameters:
 *  list - the list to remove the item from
 *  item - the pointer to the item to be removed
 *
 *  Returns: A pointer to the list that had the item removed from
 **/
LinkedList* linked_list_remove_item(LinkedList* list, void* item);

/**
 * A debugging method to print all the elements of a list.
 * Parameters:
 *  list - the list to print 
 *  stream - the output stream to print to
 *  formatString - the format string for how to format the items
 * 
 * Returns: None
 * */
void linked_list_print(LinkedList* list, FILE* stream, char* formatString);

/**
 *  Frees the pointer to the list.
 *  Note any memory allocated to the items is not freed.
 *  
 *  Parameters:
 *      list - the list to free
 *
 *  Returns: None
 * */
void linked_list_free(LinkedList* list);

#endif
