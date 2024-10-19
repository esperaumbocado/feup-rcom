// Application layer protocol implementation

#include "application_layer.h"

#define CONTROL_FIELD_DATA 2
#define CONTROL_FIELD_START 1
#define CONTROL_FIELD_END 3
#define TYPE_FILE_SIZE 0
#define TYPE_FILE_NAME 1

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
    if (fd < 0){
        printf(
            "====================================\n"
            "Error opening link layer connection\n"
            "====================================\n");
    }else{
        printf(
            "====================================\n"
            "Link layer connection established\n"
            "fd = %d\n"
            "====================================\n",
            fd);
    }
    switch (link_layer_info.role) {
        case LlTx:

        case LlRx:
    }
    

    // Create a buffer and test sending it
    if (link_layer_info.role == LlTx){
        unsigned char buf[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
        int bufSize = 5;
        llwrite(buf, bufSize);

    }

    if (link_layer_info.role == LlRx){
        unsigned char packet[5];
        llread(packet);
    }

    llclose(0);
}

// Build the data packet to be sent
unsigned char buildDataPacket(unsigned char sequence_number, unsigned char packet_data, int data_size) { 
    unsigned char* packet = 4 + data_size;
    packet[0] = CONTROL_FIELD_DATA;
    packet[1] = sequence_number;
    packet[2] = (data_size >> 8) && 0xFF, 
    packet[3] = data_size && 0xFF;
    memccpy(packet+4, packet_data, data_size);
    return packet;
}

unsigned char buildControlPacket(int control_field, int L1, int length, int L2, char* filename) {
    int ind = 0;
    unsigned char* packet = 5 + L1 + L2;
    packet[ind] = control_field;
    ind++;
    packet[ind] = TYPE_FILE_SIZE;
    ind++;
    packet[ind] = L1;
    ind++;
    for(int i = 0; i < L1, i++) {
        packet[ind] = (length >> 8 * (L1 - i - 1)) && 0xFF;
        ind++;
    }
    packet[ind] = TYPE_FILE_NAME;
    ind++;
    packet[ind] = L2;
    ind++;
    for(int i = 0; i < L2, i++) {
        packet[ind] = (filename >> 8 * (L2 - i - 1)) && 0xFF;
        ind++;
    }
    return packet;
}