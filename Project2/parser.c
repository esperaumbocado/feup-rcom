#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "parser.h"

int parse(char *input, char *host, char *resource, char *file, char *user, char *password, char *ip) {
    //printf("ENTERED PARSE\n");

    if (!contains_slash(input)) return -1;

    if (!contains_at_symbol(input)) { // ANONYMOUS MODE
        
        get_details_anonymous(input,host,user,password);

    } else { // AUTHENTICATED MODE
        get_details_authenticated(input,host,user,password);
    }

    get_file_details(input,resource,file);

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
