// Application layer protocol implementation

#include "application_layer.h"

#define FLAG 0x7E

#define A_TX 0x03
#define A_RX 0x01

// Supervision and Unnumbered frame related definitions
#define C_SET 0x03
#define C_UA 0x07
#define C_RR0 0xAA
#define C_RR1 0xAB
#define C_REJ0 0x54
#define C_REJ1 0x55
#define C_DISC 0x0B

// BCC related definitions
#define BCC1(A, C) (A ^ C)

// Information frame related definitions
#define C_FRAME0 0x00
#define C_FRAME1 0x80

// More definitions
#define ESCAPE 0x7D

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
    switch (link_layer_info.role) {
        case LlTx:
            unsigned char pac[9] = {0x67, 0x23, 0x7E,};
            llwrite(pac ,3);
            break;
        case LlRx:
            unsigned char bj;
            llread(&bj);
            break;
    }
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
}