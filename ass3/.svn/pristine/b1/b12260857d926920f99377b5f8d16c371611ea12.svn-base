#include <stdbool.h>

#include "common.h"

/****
 *
 * Some of the utility methods here are adapted from assignment 1
 *
 * */

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
 **/
char* read_line(FILE *file, int readChunk);

/**
 * Initialises the list used to hold lines.
 *
 * Parameters: None
 * 
 * Returns: The initialised lines list
 * */
LinesList* lines_list_init();

/**
 * 
 * Adds a new message to the list.
 *
 * Parameters:
 *  list - the list to add the message to
 *  message - the message to add to the list
 *
 *  Returns: None
 */
void lines_list_add(LinesList* list, char* message);

/**
 *
 * Cleans up memory allocated to the list.
 * It's the responsibility of the user to free the lines
 *
 * Parameters:
 *  list - the list to clean up 
 *
 * Returns: None
 */
void lines_list_free(LinesList* list);

/**
 * Prints the programs usage. 
 * 
 * Parameters:
 *  file - The file to print to
 *  message - The message to print
 *  exitCode - The exit code number to exit the program with
 *
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
 * Returns: The parsed message in a struct pointer which must be freed
 * after use.
 * */
ParsedMessage* parse_message(char* message);

/**
 * Frees memory allocated to a ParsedMessage struct.
 *
 * Parameters:
 *  parsedMessage* - the ParsedMesage pointer to have resources freed
 * Returns: None
 * */
void free_parsed_message(ParsedMessage* parsedMessage);

/**
 *  Checks whether a given is a comment line.
 *
 *  Parameters:
 *      line - the line to check
 *  Returns: true if the line is a comment false otherwise
 * */
bool is_comment_line(char* line);
