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
    if (fd < 0){
        printf(
            "====================================\n"
            "Error opening link layer connection\n"
            "====================================\n");
            return;
    }else{
        printf(
            "====================================\n"
            "Link layer connection established\n"
            "fd = %d\n"
            "====================================\n",
            fd);
    }
    

    // Create a buffer and test sending it
    if (link_layer_info.role == LlTx){
        unsigned char buf[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
        //calculate BCC2 of the buffer
        unsigned char BCC2 = 0;
        for (int i = 0; i < 5; i++){
            BCC2 ^= buf[i];
        }
        printf("BCC2: 0x%02X\n", BCC2);
        int bufSize = 5;
        llwrite(buf, bufSize);

    }

    if (link_layer_info.role == LlRx){
        unsigned char * packet = (unsigned char *) malloc(1000);
        llread(packet);
        //print packet
        for (int i = 0; i < 1000; i++){
            printf("Ox%02X ", packet[i]);
        }
    }

    llclose(0);
}