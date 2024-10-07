// Application layer protocol implementation

#include "application_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer link_layer_info;
    strcpy(link_layer_info.serialPort, serialPort);
    link_layer_info.baudRate = baudRate;
    link_layer_info.role = strcmp(role, "tx") ? LlRx : LlTx;
    link_layer_info.nRetransmissions = nTries;
    link_layer_info.timeout = timeout;
    int fd = llopen(link_layer_info);
    printf("fd = 0x%02X\n", fd);
}
