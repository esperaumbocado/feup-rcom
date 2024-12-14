#ifndef TRANSFER_H
#define TRANSFER_H

#include "constants.h"
#include "socket.h"

int requestResource(const int socket, char *resource);
int getResource(const int socketA, const int socketB, char *filename);

#endif
