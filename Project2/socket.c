#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
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

    char userCommand[5 + strlen(user) + 2 + 1]; 
    sprintf(userCommand, "user %s\r\n", user);
    char passCommand[5 + strlen(pass) + 2 + 1]; 
    sprintf(passCommand, "pass %s\r\n", pass);
    char answer[MAX_LENGTH];
    
    sendCommand(socket, userCommand);
    int readResponseValue = readResponse(socket,answer);
    printf("COMMAND SENT | RESPONSE VALUE: %d", readResponseValue);
    if (readResponseValue != READY4PASS_CODE) {
        printf("Unknown user '%s'. Abort.\n", user);
        exit(-1);
    }

    printf("USER OK\n");

    sendCommand(socket, passCommand);
    return readResponse(socket, answer);
}

int enterPassiveMode(const int socket, char *ip, int *port) {
    printf("ENTERED PASSIVE MODE\n");

    char answer[MAX_LENGTH];
    int ip1, ip2, ip3, ip4, port1, port2;
    char passiveCommand[7] = "pasv\r\n";
    sendCommand(socket, passiveCommand);
    if (readResponse(socket, answer) != PASSIVE_CODE) 
        return -1;

    sscanf(answer, "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]", &ip1, &ip2, &ip3, &ip4, &port1, &port2);
    *port = port1 * 256 + port2;
    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

    return PASSIVE_CODE;
}

int readResponse(const int socket, char *buffer) {
    printf("ENTERED READ RESPONSE\n");
    char byte;
    int index = 0;
    int responseCode = 0;

    printf("STARTED BUFF CLEAR\n");
    memset(buffer, 0, MAX_LENGTH);
    printf("ENDED BUFF CLEAR\n");

    while (1) {
        if (read(socket, &byte, 1) > 0) {
            printf("BYTE: %c\n", byte);
            buffer[index++] = byte;
            buffer[index] = '\0';

            if (index >= MAX_LENGTH - 1) {
                fprintf(stderr, "Error: Line too long or malformed response.\n");
                close(socket);
                exit(-1);
            }

            if (byte == '\n') {
                if (index >= 4 && isdigit(buffer[0]) && isdigit(buffer[1]) &&
                    isdigit(buffer[2]) && buffer[3] == ' ') {
                    sscanf(buffer, "%d", &responseCode); 
                    break;
                }
                index = 0; 
            }
        } else {
            perror("read()");
            close(socket);
            exit(-1);
        }
    }

    printf("Response: %s", buffer);
    printf("Response code: %d\n", responseCode);
    return responseCode;
}


int closeConnection(const int socketA) {
    printf("ENTERED CLOSE CONNECTION\n");
    
    char answer[MAX_LENGTH];
    char quitCommand[7] = "quit\r\n";
    sendCommand(socketA, quitCommand);
    int readResponseValue = readResponse(socketA,answer);
    if (readResponseValue != GOODBYE_CODE) 
        return -1;
    return close(socketA);
}
