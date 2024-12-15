#include "utils.h"

void sendCommand(const int socket, const char* command) {
    printf("Command: %s",command);
    if(write(socket, command, strlen(command))<=0){
        printf("ERROR WRITING COMMAND\n");
    }
}

int contains_at_symbol(const char *str) {
    return strchr(str, '@') != NULL;
}

int contains_slash(const char *str) {
    return strchr(str, '/') != NULL;
}

void get_details_authenticated(char *input, char *host, char *user, char*password){
    sscanf(input, "%*[^/]//%*[^@]@%[^/]", host);
    sscanf(input, "%*[^/]//%[^:/]", user);
    sscanf(input, "%*[^/]//%*[^:]:%[^@\n$]", password);
}

void get_details_anonymous(char *input, char *host, char *user, char *password){
    sscanf(input, "%*[^/]//%[^/]", host);
    strcpy(user, DEFAULT_USER);
    strcpy(password, DEFAULT_PASSWORD);
}

void get_file_details(char *input, char *resource, char *file){
    sscanf(input, "%*[^/]//%*[^/]/%s", resource);
    strcpy(file, strrchr(input, '/') + 1);
}
