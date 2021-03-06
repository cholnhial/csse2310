#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include "utils.h"

int compare_words(const void *firstWord, const void *secondWord) {
    return strcasecmp(*(char**) firstWord, *(char**) secondWord);
}

char *read_line(FILE *file, int readChunk) {
    int chunkSize = readChunk;
    int characterPosition = 0;
    char *line = malloc(readChunk);
    int charRead;
    while ((charRead = fgetc(file)) != '\n') {
        if (charRead == EOF) {
            return NULL; // indicate we are done reading
        }
        line[characterPosition++] = charRead;
        if (characterPosition == chunkSize) {
            chunkSize += readChunk;
            line = realloc(line, chunkSize);
        }
    }
    line[characterPosition] = 0;

    return line;
}

DictionaryWords *dict_words_init() {
    DictionaryWords *dict = (DictionaryWords*) malloc(sizeof(DictionaryWords));
    dict->words = (char**) malloc(sizeof(char*));
    dict->size = 0;
    dict->memsize = sizeof(char*);

    return dict;
}

void dict_words_add(DictionaryWords *dict, char *word) {
    dict->words[dict->size] = word;
    int newSize = dict->memsize + sizeof(char*);
    dict->memsize = newSize;
    dict->words = (char**) realloc(dict->words, newSize);
    dict->size++;
}

void dict_words_free(DictionaryWords *dict) {
    if (dict->words != NULL) {
        free(dict->words);
    }
    if (dict != NULL) {
        free(dict);
        dict = NULL;
    }
}
