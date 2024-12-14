#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "transfer.h"

int requestResource(const int socket, char *resource) {
    printf("ENTERED REQUEST RESOURCE\n");

    char fileCommand[5+strlen(resource)+1], answer[MAX_LENGTH];
    sprintf(fileCommand, "retr %s\n", resource);
    sendCommand(socket, fileCommand);
    return readResponse(socket, answer);
}

int getResource(const int socketA, const int socketB, char *filename) {
    printf("ENTERED GET RESOURCE\n");

    FILE *fd = fopen(filename, "wb");
    if (fd == NULL) {
        printf("Error opening or creating file '%s'\n", filename);
        exit(-1);
    }

    char buffer[MAX_LENGTH];
    int bytes;
    do {
        bytes = read(socketB, buffer, MAX_LENGTH);
        if (fwrite(buffer, bytes, 1, fd) < 0) return -1;
    } while (bytes);
    fclose(fd);

    return readResponse(socketA, buffer);
}
