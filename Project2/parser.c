#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "parser.h"

int parse(char *input, struct URL *url) {
    printf("ENTERED PARSE\n");

    regex_t regex;
    regcomp(&regex, "/", 0);
    if (regexec(&regex, input, 0, NULL, 0)) return -1;

    regcomp(&regex, "@", 0);
    if (regexec(&regex, input, 0, NULL, 0) != 0) { // ANONYMOUS MODE
        
        sscanf(input, "%*[^/]//%[^/]", url->host);
        strcpy(url->user, DEFAULT_USER);
        strcpy(url->password, DEFAULT_PASSWORD);

    } else { // AUTHENTICATED MODE

        sscanf(input, "%*[^/]//%*[^@]@%[^/]", url->host);
        sscanf(input, "%*[^/]//%[^:/]", url->user);
        sscanf(input, "%*[^/]//%*[^:]:%[^@\n$]", url->password);
    }

    sscanf(input, "%*[^/]//%*[^/]/%s", url->resource);
    strcpy(url->file, strrchr(input, '/') + 1);

    struct hostent *h;
    if (strlen(url->host) == 0) return -1;
    if ((h = gethostbyname(url->host)) == NULL) {
        printf("Invalid hostname '%s'\n", url->host);
        exit(-1);
    }
    strcpy(url->ip, inet_ntoa(*((struct in_addr *) h->h_addr)));

    return !(strlen(url->host) && strlen(url->user) && 
           strlen(url->password) && strlen(url->resource) && strlen(url->file));
}
