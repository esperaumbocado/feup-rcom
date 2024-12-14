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

    struct URL url;
    memset(&url, 0, sizeof(url));
    if (parse(argv[1], &url) != 0) {
        printf("Parse error. Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }
    
    printf("-------------------\n URL INFO\n-------------------\nHost: %s\nResource: %s\nFile: %s\nUser: %s\nPassword: %s\nIP Address: %s\n-------------------\n", url.host, url.resource, url.file, url.user, url.password, url.ip);

    char answer[MAX_LENGTH];
    int socketA = createSocket(url.ip, FTP_PORT);
    if (socketA < 0 || readResponse(socketA, answer) != READY4AUTH_CODE) {
        printf("Socket to '%s' and port %d failed\n", url.ip, FTP_PORT);
        exit(-1);
    }
    
    if (authFTP(socketA, url.user, url.password) != LOGINSUCCESS_CODE) {
        printf("Authentication failed with username = '%s' and password = '%s'.\n", url.user, url.password);
        exit(-1);
    }
    
    int port;
    char ip[MAX_LENGTH];
    if (enterPassiveMode(socketA, ip, &port) != PASSIVE_CODE) {
        printf("Passive mode failed\n");
        exit(-1);
    }

    int socketB = createSocket(ip, port);
    if (socketB < 0) {
        printf("Socket to '%s:%d' failed\n", ip, port);
        exit(-1);
    }

    if (requestResource(socketA, url.resource) != READY4TRANSFER_CODE) {
        printf("Unknown resouce '%s' in '%s:%d'\n", url.resource, ip, port);
        exit(-1);
    }

    if (getResource(socketA, socketB, url.file) != TRANSFER_COMPLETE_CODE) {
        printf("Error transfering file '%s' from '%s:%d'\n", url.file, ip, port);
        exit(-1);
    }

    if (closeConnection(socketA, socketB) != 0) {
        printf("Sockets close error\n");
        exit(-1);
    }

    return 0;
}
