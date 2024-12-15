#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "socket.h"
#include "transfer.h"
#include <assert.h>
#include <unistd.h>


typedef enum {
    STATE_INIT,
    STATE_PARSE_URL,
    STATE_CONNECT_CONTROL,
    STATE_AUTHENTICATE,
    STATE_ENTER_PASSIVE,
    STATE_CONNECT_DATA,
    STATE_REQUEST_RESOURCE,
    STATE_DOWNLOAD_RESOURCE,
    STATE_CLOSE_CONNECTION,
    STATE_SUCCESS,
    STATE_ERROR
} State;

int main(int argc, char *argv[]) {
    State currentState = STATE_INIT;
    char host[MAX_LENGTH], resource[MAX_LENGTH], file[MAX_LENGTH], user[MAX_LENGTH], password[MAX_LENGTH], ip[MAX_LENGTH];
    char ip_pass[MAX_LENGTH], answer[MAX_LENGTH];
    int socketA = -1, socketB = -1, port = 0;

    while (currentState != STATE_SUCCESS && currentState != STATE_ERROR) {
        switch (currentState) {
            case STATE_INIT:
                if (argc != 2) {
                    printf("Use: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
                    currentState = STATE_ERROR;
                } else {
                    currentState = STATE_PARSE_URL;
                }
                break;

            case STATE_PARSE_URL:
                if (parse(argv[1], host, resource, file, user, password, ip) != 0) {
                    printf("Error parsing URL. Use: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
                    currentState = STATE_ERROR;
                } else {
                    printf("-------------------\n URL INFO\n-------------------\nHost: %s\nResource: %s\nFile: %s\nUser: %s\nPassword: %s\nIP Address: %s\n-------------------\n",
                           host, resource, file, user, password, ip);
                    currentState = STATE_CONNECT_CONTROL;
                }
                break;

            case STATE_CONNECT_CONTROL:
                socketA = createSocket(ip, FTP_PORT);
                if (socketA < 0 || readResponse(socketA, answer) != READY4AUTH_CODE) {
                    printf("Failed creating socket to ip '%s' and port %d\n", ip, FTP_PORT);
                    currentState = STATE_ERROR;
                } else {
                    currentState = STATE_AUTHENTICATE;
                }
                break;

            case STATE_AUTHENTICATE:
                if (authFTP(socketA, user, password) != LOGINSUCCESS_CODE) {
                    printf("Failed to authenticate: username = '%s' and password = '%s'.\n", user, password);
                    currentState = STATE_ERROR;
                } else {
                    currentState = STATE_ENTER_PASSIVE;
                }
                break;

            case STATE_ENTER_PASSIVE:
                if (enterPassiveMode(socketA, ip_pass, &port) != PASSIVE_CODE) {
                    printf("Failed to enter passive mode\n");
                    currentState = STATE_ERROR;
                } else {
                    currentState = STATE_CONNECT_DATA;
                }
                break;

            case STATE_CONNECT_DATA:
                socketB = createSocket(ip_pass, port);
                if (socketB < 0) {
                    printf("Failed creating socket to ip '%s:%d' failed\n", ip_pass, port);
                    currentState = STATE_ERROR;
                } else {
                    currentState = STATE_REQUEST_RESOURCE;
                }
                break;

            case STATE_REQUEST_RESOURCE:
                {
                    int requestResourceValue = requestResource(socketA, resource);
                    //Uprintf("REQUEST RESOURCE VALUE %d\n", requestResourceValue);
                    if (requestResourceValue != READY4TRANSFER_CODE && requestResourceValue != READY4TRANSFER_CODE2) {
                        printf("Resource not found'%s' in '%s:%d'\n", resource, ip_pass, port);
                        currentState = STATE_ERROR;
                    } else {
                        currentState = STATE_DOWNLOAD_RESOURCE;
                    }
                }
                break;

            case STATE_DOWNLOAD_RESOURCE:
                if (getResource(socketA, socketB, file) != TRANSFER_COMPLETE_CODE) {
                    printf("Error downloading the file '%s' from ip '%s:%d'\n", file, ip_pass, port);
                    currentState = STATE_ERROR;
                } else {
                    currentState = STATE_CLOSE_CONNECTION;
                }
                break;

            case STATE_CLOSE_CONNECTION:
                if (closeConnection(socketA) != 0) {
                    printf("Error closing controller socket\n");
                    currentState = STATE_ERROR;
                } else {
                    currentState = STATE_SUCCESS;
                }
                break;

            default:
                currentState = STATE_ERROR;
                break;
        }
    }

    if (currentState == STATE_ERROR) {
        if (socketA >= 0) close(socketA);
        if (socketB >= 0) close(socketB);
        printf("An error occurred during the FTP download.\n");
        return -1;
    }

    printf("-------------------\nFTP download completed successfully.\n");
    return 0;
}