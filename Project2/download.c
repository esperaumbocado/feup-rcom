#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "socket.h"
#include "transfer.h"
#include <assert.h>
#include <unistd.h>


int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    } 

    char host[MAX_LENGTH], resource[MAX_LENGTH], file[MAX_LENGTH], user[MAX_LENGTH], password[MAX_LENGTH], ip[MAX_LENGTH];


    if (parse(argv[1], host, resource, file, user, password, ip) != 0) {
        printf("Parse error. Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }
    
    printf("-------------------\n URL INFO\n-------------------\nHost: %s\nResource: %s\nFile: %s\nUser: %s\nPassword: %s\nIP Address: %s\n-------------------\n", host, resource, file, user, password, ip);

    char answer[MAX_LENGTH];
    int socketA = createSocket(ip, FTP_PORT);
    if (socketA < 0 || readResponse(socketA, answer) != READY4AUTH_CODE) {
        printf("Socket to '%s' and port %d failed\n", ip, FTP_PORT);
        exit(-1);
    }
    
    if (authFTP(socketA, user, password) != LOGINSUCCESS_CODE) {
        printf("Authentication failed with username = '%s' and password = '%s'.\n", user, password);
        exit(-1);
    }
    
    int port;
    char ip_pass[MAX_LENGTH];
    if (enterPassiveMode(socketA, ip_pass, &port) != PASSIVE_CODE) {
        printf("Passive mode failed\n");
        exit(-1);
    }

    int socketB = createSocket(ip_pass, port);
    if (socketB < 0) {
        printf("Socket to '%s:%d' failed\n", ip_pass, port);
        exit(-1);
    }

    
    int requestResourceValue = requestResource(socketA,resource);
    printf("REQUEST RESOURCE VALUE %d\n", requestResourceValue);
    if (requestResourceValue != READY4TRANSFER_CODE && requestResourceValue != READY4TRANSFER_CODE2) {
        printf("Unknown resouce '%s' in '%s:%d'\n", resource, ip_pass, port);
        exit(-1);
    }

    if (getResource(socketA, socketB, file) != TRANSFER_COMPLETE_CODE) {
        printf("Error transfering file '%s' from '%s:%d'\n", file, ip_pass, port);
        exit(-1);
    }

    if (closeConnection(socketA) != 0) {
        printf("Sockets close error\n");
        exit(-1);
    }

    return 0;
}
