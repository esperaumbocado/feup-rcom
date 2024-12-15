#ifndef SOCKET_H
#define SOCKET_H

#include "constants.h"
#include "utils.h"

int createSocket(char *ip, int port);
int authFTP(const int socket, const char* user, const char* pass);
int enterPassiveMode(const int socket, char *ip, int *port);
int readResponse(const int socket, char *buffer);
int closeConnection(const int socketA);

#endif
