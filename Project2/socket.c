#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "socket.h"

int createSocket(char *ip, int port) {
    printf("ENTERED CREATE SOCKET\n");

    int sockfd;
    struct sockaddr_in server_addr;

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);  
    server_addr.sin_port = htons(port); 
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }
    
    return sockfd;
}

int authFTP(const int socket, const char* user, const char* pass) {
    printf("ENTERED AUTHCONN\n");

    char userCommand[5 + strlen(user) + 1]; 
    sprintf(userCommand, "user %s\n", user);
    char passCommand[5 + strlen(pass) + 1]; 
    sprintf(passCommand, "pass %s\n", pass);
    char answer[MAX_LENGTH];
    
    sendCommand(socket, userCommand);
    if (readResponse(socket, answer) != READY4PASS_CODE) {
        printf("Unknown user '%s'. Abort.\n", user);
        exit(-1);
    }

    sendCommand(socket, passCommand);
    return readResponse(socket, answer);
}

int enterPassiveMode(const int socket, char *ip, int *port) {
    printf("ENTERED PASSIVE MODE\n");

    char answer[MAX_LENGTH];
    int ip1, ip2, ip3, ip4, port1, port2;
    char passiveCommand[5] = "pasv\n";
    sendCommand(socket, passiveCommand);
    if (readResponse(socket, answer) != PASSIVE_CODE) 
        return -1;

    sscanf(answer, "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]", &ip1, &ip2, &ip3, &ip4, &port1, &port2);
    *port = port1 * 256 + port2;
    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

    return PASSIVE_CODE;
}

int readResponse(const int socket, char* buffer) {
    printf("ENTERED READ RESPONSE\n");

    char byte;
    int index = 0, responseCode;
    ResponseState state = START;
    memset(buffer, 0, MAX_LENGTH);

    while (state != END) {
        
        read(socket, &byte, 1);
        switch (state) {
            case START:
                if (byte == ' ') 
                    state = SINGLE;
                else if (byte == '-') 
                    state = MULTIPLE;
                else if (byte == '\n') 
                    state = END;
                else 
                    buffer[index++] = byte;
                break;
            case SINGLE:
                if (byte == '\n') 
                    state = END;
                else 
                    buffer[index++] = byte;
                break;
            case MULTIPLE:
                if (byte == '\n') {
                    memset(buffer, 0, MAX_LENGTH);
                    state = START;
                    index = 0;
                }
                else 
                    buffer[index++] = byte;
                break;
            case END:
                break;
            default:
                break;
        }
    }

    sscanf(buffer, "%d", &responseCode);
    printf("-------------------\nResponse Code: %d\n-------------------\n", responseCode);
    return responseCode;
}

int closeConnection(const int socketA, const int socketB) {
    printf("ENTERED CLOSE CONNECTION\n");
    
    char answer[MAX_LENGTH];
    char quitCommand[5] = "quit\n";
    sendCommand(socketA, quitCommand);
    if (readResponse(socketA, answer) != GOODBYE_CODE) 
        return -1;
    return close(socketA) || close(socketB);
}
