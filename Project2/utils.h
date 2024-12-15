#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "constants.h"

void sendCommand(const int socket, const char* command);
int contains_at_symbol(const char* input);
int contains_slash(const char* input);
void get_details_authenticated(char *input, char *host, char *user, char*password);
void get_details_anonymous(char *input, char *host, char *user, char *passowrd);
void get_file_details(char *input, char *resource, char *file);
#endif
