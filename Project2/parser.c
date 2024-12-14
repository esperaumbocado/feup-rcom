#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "parser.h"

int parse(char *input, char *host, char *resource, char *file, char *user, char *password, char *ip) {
    printf("ENTERED PARSE\n");

    regex_t regex;
    regcomp(&regex, "/", 0);
    if (regexec(&regex, input, 0, NULL, 0)) return -1;

    regcomp(&regex, "@", 0);
    if (regexec(&regex, input, 0, NULL, 0) != 0) { // ANONYMOUS MODE
        
        sscanf(input, "%*[^/]//%[^/]", host);
        strcpy(user, DEFAULT_USER);
        strcpy(password, DEFAULT_PASSWORD);

    } else { // AUTHENTICATED MODE

        sscanf(input, "%*[^/]//%*[^@]@%[^/]", host);
        sscanf(input, "%*[^/]//%[^:/]", user);
        sscanf(input, "%*[^/]//%*[^:]:%[^@\n$]", password);
    }

    sscanf(input, "%*[^/]//%*[^/]/%s", resource);
    strcpy(file, strrchr(input, '/') + 1);

    struct hostent *h;
    if (strlen(host) == 0) return -1;
    if ((h = gethostbyname(host)) == NULL) {
        printf("Invalid hostname '%s'\n", host);
        exit(-1);
    }
    strcpy(ip, inet_ntoa(*((struct in_addr *) h->h_addr)));

    return !(strlen(host) && strlen(user) && 
           strlen(password) && strlen(resource) && strlen(file));
}
