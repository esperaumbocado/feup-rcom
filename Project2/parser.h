#ifndef PARSER_H
#define PARSER_H

#include "constants.h"

struct URL {
    char host[MAX_LENGTH];
    char resource[MAX_LENGTH];
    char file[MAX_LENGTH];
    char user[MAX_LENGTH];
    char password[MAX_LENGTH];
    char ip[MAX_LENGTH];
};

int parse(char *input, struct URL *url);

#endif
