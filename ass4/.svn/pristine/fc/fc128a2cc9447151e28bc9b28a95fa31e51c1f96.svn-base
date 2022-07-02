#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "utils.h"

void terminate_on_communication_error() {
    fprintf(stderr, "Communications error\n");
    exit(EXIT_COMMUNICATION_ERROR);
}

char* read_auth_secret(char* filename) {

    FILE* file = fopen(filename, "r");
    return  read_line(file, READ_MEM_ALLOCATION_CHUNK);
}

void print_usage(FILE* file, char* message, int exitCode) {
    fprintf(file, message);

    exit(exitCode);
}
