#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "utils.h"
#include "common.h"

/* The default file to read words from when 
 * user hasn't specified a file name */
#define DEFAULT_DICTIONARY_FILENAME "/usr/share/dict/words"

/* We dynammically resize the word read, we allocate in chunks for speed*/
#define READ_MEM_ALLOCATION_CHUNK 32

/* Enum representing program search type */
typedef enum {
    SEARCH_PREFIX, SEARCH_EXACT, SEARCH_ANYWHERE, BAD_OPTION, SORT_OPTION
} OptionType;

/* A structure that represents the program options */
typedef struct {
    OptionType searchType;
    bool sort;
    char *pattern;
    char *dictionaryFilename;
    bool searchExactFound; 
    bool searchPrefixFound;
    bool searchAnywhereFound;
    bool isSortSpecified;
} Options;

/**
 * Prints the program usage and exits with a specified error code.
 *
 * Params:
 *  stream - The file stream to output the usage message to
 *  exitCode - The exit code which to exit with and terminate execution
 *
 *  Returns: nothing
 */
void print_usage(FILE *stream, int exitCode) {
    fprintf(stream, "Usage: search [-exact|-prefix|-anywhere]"
            " [-sort] pattern [filename]\n");
    exit(exitCode);
}

/**
 * Checks whether a command-line argument is an options.
 *
 * Parameters:
 *  argument - The argument string to check if it starts with "-"
 *
 *  Returns: true if it's an option, false otherwise.
 */
bool is_argument_an_option(char *argument) {
    if (strstr(argument, "-") != NULL) {
        return true;
    }

    return false;
}

/**
 * Maps a command-line option to its type.
 *
 * BAD_OPTION is returned if it's an option not expected.
 *
 * Parameters:
 *  option - The command line argument to check
 *
 *  Returns an OptionType enum
 *
 * */
OptionType get_option_type(char *option) {

    if (!strcmp(option, "-exact")) {
        return SEARCH_EXACT;         
    } else if (!strcmp(option, "-prefix")) {
        return SEARCH_PREFIX;
    } else if (!strcmp(option, "-anywhere")) {
        return SEARCH_ANYWHERE;
    } else if (!strcmp(option, "-sort")) {
        return SORT_OPTION; 
    } else {
        return BAD_OPTION;
    }
}

/**
 *
 * Processes the command-line argument and performs some validation.
 * Exiting int the process is a violation is foumd.
 *
 * Parameters:
 *  argument - the argument to check
 *  options - the programs options structure.
 *
 *  Returns nothing
 */
void process_argument(char *argument, Options *options) {
    OptionType searchType = get_option_type(argument);
    switch (searchType) {

        case SEARCH_EXACT:
            if (!options->searchExactFound) {
                options->searchExactFound = true;
            } else {
                print_usage(stderr, EXIT_FAILURE);
            }
            break;
        case SEARCH_PREFIX:
            if (!options->searchPrefixFound) {
                options->searchPrefixFound = true;
            } else {
                print_usage(stderr, EXIT_FAILURE);
            }
            break;
        case SEARCH_ANYWHERE:
            if (!options->searchAnywhereFound) {
                options->searchAnywhereFound = true;
            } else {
                print_usage(stderr, EXIT_FAILURE);
            } 
            break;
        case SORT_OPTION:
            if (!options->isSortSpecified) {
                options->isSortSpecified = true;
            } else {
                print_usage(stderr, EXIT_FAILURE);
            }

            break;
        default:
            print_usage(stderr, EXIT_FAILURE);

    }
}

/**
 * Finally build the command options, to later be queried.
 * The patternIndex and filenameIndex tell us where the pattern and
 * filename are in argv.
 *
 * Parameters:
 *  patternIndex - the index of the pattern in argv
 *  filenameIndex - the index of the filename in argv
 *  argv - the list of arguments passed to the program
 *  options - the programs options structure
 *
 *  Returns none
 */
void build_options(int patternIndex, int filenameIndex,
        char **argv, Options *options) {
    if (options->searchExactFound &&
            !(options->searchPrefixFound || options->searchAnywhereFound)) {
        options->searchType = SEARCH_EXACT;

    } else if (options->searchPrefixFound && 
            !(options->searchExactFound || options->searchAnywhereFound)) {
        options->searchType = SEARCH_PREFIX;
    } else if (options->searchAnywhereFound &&
            !(options->searchExactFound || options->searchPrefixFound)) {
        options->searchType = SEARCH_ANYWHERE;
    } else {
        options->searchType = SEARCH_EXACT;
    }

    options->sort = options->isSortSpecified;

    if (patternIndex != -1) {
        options->pattern = argv[patternIndex];
    } 

    if (filenameIndex != -1) {
        options->dictionaryFilename = argv[filenameIndex];
    } else {
        options->dictionaryFilename = DEFAULT_DICTIONARY_FILENAME;
    }
}

/**
 * Constructs an options struct that represents 
 * the command-line options passed.
 *
 * Paramaters:
 * argc - The total number of arguments passed to the program
 * argv - The list of arguments
 *
 * Returns NULL or a valid Options structure
 **/
Options *parse_options(int argc, char **argv) {
    if ((argc - 1) < 1) {
        return NULL; 
    }

    Options *options = (Options*) malloc(sizeof(Options)); 
    memset(options, 0, sizeof(Options));
    int patternIndex, filenameIndex;
    patternIndex = filenameIndex = -1;
    int nonOptionArgumentsFound = 0;

    /* Process command-line options*/
    for (int i = 1; i < argc; i++) {

        /* Is argument an option */
        if (is_argument_an_option(argv[i])) {
            process_argument(argv[i], options);
        } else {
            nonOptionArgumentsFound++;

            /*This is either a pattern or a file */
            if (patternIndex == -1) {
                patternIndex = i;
            }

            /* After a pattern we expect a file*/
            if (patternIndex != -1 && 
                    filenameIndex == -1 && patternIndex != i) {
                filenameIndex = i;
            }
        }
    }

    if (nonOptionArgumentsFound > 2 || patternIndex == -1) {
        print_usage(stderr, EXIT_FAILURE);
    }

    build_options(patternIndex, filenameIndex, argv, options);

    return options;
}

/**
 * Validates the pattern the user specified.
 * 
 * Parameters:
 * pattern - the pattern to validate
 *
 * Return true if invalid, false otherwise
 *
 * */
bool is_valid_pattern(char *pattern) {
    for (int i = 0; i < strlen(pattern); i++) {
        if (pattern[i] != '?' && !isalpha(pattern[i])) {
            return false;
        }
    }

    return true;
}

/**
 * 
 * Checks whether the file exists or can be read.
 * Exits with an error code indicating failure.
 *
 * Returns nothing
 *
 * */
void exit_on_incorrect_file_access(char *filename) {

    /* Check if file exists and if we can read */
    int returnValue = access(filename, F_OK | R_OK);

    if (returnValue == -1) {
        fprintf(stderr, "search: file \"%s\" can not be opened\n", filename);
        exit(EXIT_FAILURE);
    }
}

/**
 * Checks whether a pattern is to match prefix "????...?" 
 *  
 *  Parameters:
 *   pattern - THe pattern to check if it's a match all prefix
 *
 * Returns true if this is a match any prfix, false otherwise
 * */
bool is_match_all_prefix(char *pattern) {
    for (int i = 0; i < strlen(pattern); i++) {
        if (pattern[i] != '?') {
            return false;
        }
    }

    return true;
}

/**
 *  Checks whether a word is all alphabetic
 *
 *  Parameters:
 *      word - The word to check if it's all alphabetic
 *
 *  Returns true if the word is alphabetic false otherwise
 * */
bool is_all_alphabetic_word(char *word) {
    for (int i = 0; i < strlen(word); i++) {
        if (!isalpha(word[i])) {
            return false;
        }
    }

    return true;
}

/**
 *
 *  This function is responsible for handling prefix matches
 *
 *  Parameters:
 *      word - The word to check if it matches a prefix
 *      pattern - The pattern containing the prefix
 *
 *  Returns true if the pattern matches, false otherwise 
 **/
bool is_word_a_prefix_match(char *word, char *pattern) {

    if (strlen(word) >= strlen(pattern) && is_all_alphabetic_word(word)) {

        if (is_match_all_prefix(pattern)) {
            return true;
        }

        for (int i = 0; i < strlen(pattern); i++) {
            if (!isalpha(word[i])) {
                return false;    
            }

            if (pattern[i] == '?') {
                continue;
            }

            if (isalpha(word[i]) && tolower(pattern[i]) == tolower(word[i])) {
                continue;
            } else {
                return false;
            }          
        }

        return true;
    }

    return false;
}

/**
 * Goes through the dictionary to check all words that match the prefix
 *
 * Parameters:
 *  pattern - The pattern that contains the prefix
 *  dict - The dictionary containing all the words to check against
 *
 *  Returns a dictionary containing the list that contains the matched words 
 *
 * */
DictionaryWords *pattern_match_words_prefix(char *pattern, 
        DictionaryWords *dict) {
    DictionaryWords *matchesDict = dict_words_init();

    for (int i = 0; i < dict->size; i++) {
        if (is_word_a_prefix_match(dict->words[i], pattern)) {
            dict_words_add(matchesDict, dict->words[i]); 
        }
    }

    return matchesDict;
}

/**
 * Check whether two letters match. Will return true if letter is being
 * compared with '?'
 *  
 *  Parameters:
 *   patternLetter   - the letter of the pattern to compare
 *   wordLetter      - the letter in the word to compare 
 *   
 * Returns true if they match or is a comparison again '?', false otherwise
 */
bool is_letter_a_match(char patternLetter, char wordLetter) {

    if (patternLetter == '?') {
        return true;
    }

    if (isalpha(wordLetter) && tolower(patternLetter) == tolower(wordLetter)) {
        return true;

    }

    return false;
}

/**
 * Find the next beginning of the word that matches
 * the first letter of the pattern.
 *
 * Parameters:
 *  patern - The pattern
 *  word - The word we will search for the beginning of the pattern
 *  lastIndex - Where we will start looking for the start in the word
 *
 * Returns the index to where the word starts, -1 if none can be found
 */
int get_start_of_next_match_index(char *pattern, char *word, int lastIndex) {

    bool lastLetterMatched = false;

    /*Seek for first letter that matches*/
    int i = lastIndex;
    while (i < strlen(word)) {
        lastLetterMatched = is_letter_a_match(pattern[0], word[i]); 
        if (lastLetterMatched && tolower(pattern[0]) !=  tolower(word[i+1])) {
            break;
        }
        i++;
    }

    return lastLetterMatched ? i : -1;
}

/**
 * This is the algorithm to try and match parts of a word 
 * with the anywhere search strategy. It First tried
 * to locate an index in the word where the first character matches
 * the first character of the pattern. It will keep doing this until
 * all the places starting with the first letter of the pattern are exhausted.
 *
 * Parameter:
 *  word - The word to search for the pattern
 *  pattern - The pattern to search the word by
 *
 *
 */
bool is_word_an_anywhere_match(char *word, char *pattern) {

    /* if word length is less than the pattern or word is not alphabetic,
     * no match is found */
    if (strlen(word) < strlen(pattern) || !is_all_alphabetic_word(word)) {
        return false;
    }

    int startOfMatchIndex = 0;
    int matchCount = 0;   
    while ((startOfMatchIndex = get_start_of_next_match_index(pattern,
            word, startOfMatchIndex)) != -1) {

        bool lastLetterMatched = startOfMatchIndex != -1;
        matchCount = 0;
        int i = startOfMatchIndex;
        if (startOfMatchIndex != -1) {
            int j = 0;
            do {
                bool isNewLetterAMatch = is_letter_a_match(pattern[j], 
                        word[i]);
                if (isNewLetterAMatch) {
                    matchCount++;
                }
                if (lastLetterMatched && !isNewLetterAMatch) {
                    lastLetterMatched = false;
                    break;
                }
                lastLetterMatched = isNewLetterAMatch;

                j++;
                i++;
            } while (i < strlen(word) && j < strlen(pattern));

            startOfMatchIndex = i;

            if (matchCount == strlen(pattern)) {
                break;
            }

        }

    }


    return matchCount == strlen(pattern) ? true : false;
}

/**
 * Searches word for a match of pattern anywhere in the word.
 * Will return a valid list containg matched words.
 *
 * Parameters:
 *  pattern - The pattern to look for
 *  dict - The dictionary containing the words
 *
 *  Returns a dictionary with all the matched words, which may be blank
 */
DictionaryWords *pattern_match_words_anywhere(char *pattern,
        DictionaryWords *dict) {
    DictionaryWords *matchesDict = dict_words_init();

    for (int i = 0; i < dict->size; i++) {
        if (is_word_an_anywhere_match(dict->words[i], pattern)) {
            dict_words_add(matchesDict, dict->words[i]); 
        }
    }

    return matchesDict;
}

/**
 *
 *  Checks if the word matches an exact pattern search type
 *
 *  Parameters:
 *      word - the word to check against the patter
 *      pattern - the pattern to do an exact match with
 *
 *  Returns true if the word matches, false otherwise
 *  */
bool is_word_an_exact_match(char *word, char *pattern) {

    if (strlen(word) != strlen(pattern)) {
        return false;
    }

    for (int i = 0; i < strlen(word); i++) {
        if (!isalpha(word[i])) {
            return false;    
        }
        if (pattern[i] == '?') {
            continue;
        }

        if (isalpha(word[i]) && tolower(pattern[i]) == tolower(word[i])) {
            continue;
        } else {
            return false;
        }
    }

    return true;
}

/**
 *  Searches for words that match using the exact search type.
 *  It builds a dynamic list with all the words and returns it.
 *
 *  Parameters:
 *      pattern - the pattern to match the words against
 *      dict - the dictionary of words to search 
 *
 *  Returns a dictionary containing the list of words, 
 *  might be empty, check dict->size. 
 *  
 * */
DictionaryWords *pattern_match_words_exact(char *pattern,
        DictionaryWords *dict) {
    DictionaryWords *matchesDict = dict_words_init();

    for (int i = 0; i < dict->size; i++) {

        /* We only want words that the same length */
        if (strlen(dict->words[i]) == strlen(pattern)) {
            if (is_word_an_exact_match(dict->words[i], pattern)) {
                dict_words_add(matchesDict, dict->words[i]);        
            }
        }
    }

    return matchesDict;
}

/**
 *
 *  Reads all the words from a file and puts them into the structure.
 *  The structure has the words and how many there is for easy iteration.
 *  
 *  Paramaters:
 *   filename - The file to read the words from
 *
 *   Returns the dictionary words 
 *
 * */
DictionaryWords *read_words_from_file(char *filename) {

    FILE *file = fopen(filename, "r");

    DictionaryWords *dict = dict_words_init();

    char *word;
    while ((word = read_line(file, READ_MEM_ALLOCATION_CHUNK))) {
        dict_words_add(dict, word);
    }
    return dict;
}

int main(int argc, char **argv) {

    Options *options = parse_options(argc, argv);
    if (!options) {
        print_usage(stderr, EXIT_FAILURE);
    } else {
        if (options->dictionaryFilename) {
            exit_on_incorrect_file_access(options->dictionaryFilename);
        }

        if (!is_valid_pattern(options->pattern)) {
            fprintf(stderr, "search: pattern should only" 
                    " contain question marks and letters\n");
            exit(EXIT_FAILURE);
        }

        DictionaryWords *dict = read_words_from_file(
                options->dictionaryFilename);
        DictionaryWords *matches; 
        switch (options->searchType) {
            case SEARCH_PREFIX:
                matches = pattern_match_words_prefix(options->pattern, dict);
                break;
            case SEARCH_ANYWHERE:
                matches = pattern_match_words_anywhere(options->pattern, dict);
                break;
            default:
                matches = pattern_match_words_exact(options->pattern, dict);
        }

        if (options->sort) {
            qsort(matches->words, matches->size ,sizeof(char *),
                    compare_words);
        }

        for (int i = 0; i < matches->size; i++) {
            printf("%s\n", matches->words[i]);
        }

        dict_words_free(matches);
        dict_words_free(dict);
    }

    return EXIT_SUCCESS;
}
