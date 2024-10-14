
#include "link_layer.h"
#include "serial_port.h"
#include <termios.h>
#include <unistd.h>
#include <stdio.h>


#define _POSIX_SOURCE 1 
#define SYNC_BYTE_1 0
#define ADRESS_FIELD 1
#define CONTROL_FIELD 2
#define HEADER_PROTECTION_FIELD 3
#define DATA_FIELD 4
#define DATA_PROTECTION_FIELD 5
#define SYNC_BYTE_2 6
#define FLAG 0x7E
#define A_SENDER 0X03
#define A_RECEIVER 0X01
#define BUF_SIZE 5
#define C_SET 0x03
#define C_UA 0x07
#define C_INF_0 0x00
#define C_INF_1 0x80
#define C_REJ_0 0x54
#define C_REJ_1 0x55
#define RR_0 0xAA
#define RR_1 0xAB
#define FALSE 0
#define TRUE 1
#define I_FRAME_SIZE 6
#define ERROR_OPENING -1
#define ESCAPE 0x7d

typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DATA, DATA_ESCPAPE_READ, DATA_FLAG_READ, STOP} message_state; 
typedef enum {FRAME_ACCEPTED, FRAME_REJECTED, WAITING} t_state;
message_state state = START;
t_state transmission_state = WAITING;

int alarm_cycle = 0;
int alarm_activated = FALSE;
int trys = 0;
int security_time = 0;
int Ns = 0;
int Nr = 1;

int N(int n) {
    return (n + 1) % 2;
}

void sendConnectionFrame(unsigned char A_field, unsigned char C_field) {
    unsigned char set_frame[BUF_SIZE] = {FLAG, A_field, C_field, A_field ^ C_field, FLAG};
    writeBytesSerialPort(set_frame, BUF_SIZE);
}

void alarmHandler(int signal) {
    alarm_activated = TRUE;
    alarm_cycle++;
}

int llopen(LinkLayer connectionParameters)
{
    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);
    trys = connectionParameters.nRetransmissions;
    security_time = connectionParameters.timeout;
    (void)signal(SIGALRM, alarmHandler);
    unsigned char buf;
    switch (connectionParameters.role)
    {
    case LlTx:
        unsigned char set_frame[BUF_SIZE] = {FLAG, A_SENDER, C_SET, A_SENDER ^ C_SET, FLAG};
        writeBytesSerialPort(set_frame, BUF_SIZE);
        while (trys != 0 && state != STOP)
        {
            alarm(security_time);
            alarm_activated = FALSE;
            while (alarm_activated != TRUE && state != STOP)
            {
                if(readByteSerialPort(&buf) < 0) perror("Connection error\n");
                if (state == START)
                {
                    if (buf == FLAG)
                    {
                        state = FLAG_RCV;
                        printf("START->FLAG\n");
                    }
                    else
                    {
                        state = START;
                        printf("START->START\n");
                    }
                }
                else if (state == FLAG_RCV)
                {
                    if (buf == FLAG)
                    {
                        state = FLAG_RCV;
                        printf("FLAG->FLAG\n");
                    }
                    else if (buf == A_RECEIVER)
                    {
                        state = A_RCV;
                        printf("FLAG->A_RCV\n");
                    }
                    else
                    {
                        state = START;
                        printf("FLAG->START\n");
                    }
                }
                else if (state == A_RCV)
                {
                    if (buf == FLAG)
                    {
                        state = FLAG_RCV;
                        printf("A_RCV->FLAG\n");
                    }
                    else if (buf == C_UA)
                    {
                        state = C_RCV;
                        printf("A_RCV->C_RCV\n");
                    }
                    else
                    {
                        state = START;
                        printf("A_RCV->START\n");
                    }
                }
                else if (state == C_RCV)
                {
                    if (buf == FLAG)
                    {
                        state = FLAG_RCV;
                        printf("C_RCV->FLAG\n");
                    }
                    else if (buf == (A_RECEIVER ^ C_UA))
                    {
                        state = BCC_OK;
                        printf("C_RCV->BCC\n");
                    }
                    else
                    {
                        state = START;
                        printf("C_RCV->START\n");
                    }
                }
                else if (state == BCC_OK)
                {
                    if (buf == FLAG)
                    {
                        state = STOP;
                        printf("ACABOU GG\n");
                    }
                    else
                    {
                        state = START;
                        printf("BCC->START\n");
                    }
                }
            }
            trys--;
        }
        if (state != STOP ) {
            return ERROR_OPENING;
        }
        break;

    case LlRx:
        while (trys != 0 && state != STOP)
        {
            alarm(security_time);
            alarm_activated = FALSE;
            while (alarm_activated != TRUE && state != STOP)
            {
                readByteSerialPort(&buf);
                if (state == START)
            {
                if (buf == FLAG)
                {
                    state = FLAG_RCV;
                    printf("START->FLAG\n");
                }
                else
                {
                    state = START;
                    printf("START->START\n");
                }
            }
            else if (state == FLAG_RCV)
            {
                if (buf == FLAG)
                {
                    state = FLAG_RCV;
                    printf("FLAG->FLAG\n");
                }
                else if (buf == A_SENDER)
                {
                    state = A_RCV;
                    printf("FLAG->A_RCV\n");
                }
                else
                {
                    state = START;
                    printf("FLAG->START\n");
                }
            }
            else if (state == A_RCV)
            {
                if (buf == FLAG)
                {
                    state = FLAG_RCV;
                    printf("A_RCV->FLAG\n");
                }
                else if ((buf == N(Ns)) || (buf == N(Nr)))
                {
                    state = C_RCV;
                    printf("A_RCV->C_RCV\n");
                }
                else
                {
                    state = START;
                    printf("A_RCV->START\n");
                }
            }
            else if (state == C_RCV)
            {
                if (buf == FLAG)
                {
                    state = FLAG_RCV;
                    printf("C_RCV->FLAG\n");
                }
                else if (buf == (A_SENDER ^ C_SET))
                {
                    state = BCC_OK;
                    printf("C_RCV->BCC\n");
                }
                else
                {
                    state = START;
                    printf("C_RCV->START\n");
                }
            }
            else if (state == BCC_OK)
            {
                if (buf == FLAG)
                {
                    state = STOP;
                    printf("ACABOU GG\n");
                }
                else
                {
                    state = START;
                    printf("BCC->START\n");
                }
            }
            else if ( state == DATA) {
                
            }
        }
        trys--;
        }
    }
    if (state != STOP) return ERROR_OPENING;
    unsigned char ua_frame[BUF_SIZE] = {FLAG, A_RECEIVER, C_UA, A_RECEIVER ^ C_UA, FLAG};
    writeBytesSerialPort(ua_frame, BUF_SIZE);
    return fd;
}


////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    transmission_state = WAITING;
    unsigned char b;
    int full_size = I_FRAME_SIZE + bufSize;
    for (int buf_byte = 0; buf_byte < bufSize; buf_byte++) {
        if ((buf[buf_byte] == ESCAPE) || (buf[buf_byte] == FLAG)) {
           full_size++;
        }
    }
    unsigned char i_frame[full_size];
    i_frame[SYNC_BYTE_1] = FLAG;
    i_frame[ADRESS_FIELD] = A_SENDER;
    i_frame[CONTROL_FIELD] = N(Ns);
    i_frame[HEADER_PROTECTION_FIELD] = i_frame[ADRESS_FIELD] ^ i_frame[CONTROL_FIELD];
    unsigned char BCC2 = buf[0];
    for (int bcc_byte = 1; bcc_byte < bufSize; bcc_byte++) {
        BCC2 = BCC2 ^ buf[bcc_byte];
    }
    int increment = 0;
    for (int buf_byte = 0; buf_byte < bufSize; buf_byte++) {
        if (buf[buf_byte] == ESCAPE) {
            i_frame[DATA_FIELD + buf_byte + increment] = 0x7d;
            increment++;
            full_size++;
            i_frame[DATA_FIELD + buf_byte + increment] = 0x5e;
        }
        else if (buf[buf_byte] == FLAG) {
            i_frame[DATA_FIELD + buf_byte + increment] = 0x7d;
            increment++;
            full_size++;
            i_frame[DATA_FIELD + buf_byte + increment] = 0x5d;
        }
        else {
            i_frame[DATA_FIELD + buf_byte + increment] = buf[buf_byte];
        }
    }
    i_frame[DATA_PROTECTION_FIELD + bufSize] = BCC2;
    i_frame[SYNC_BYTE_2 + bufSize] = FLAG;
    while (trys != 0)
    {
            writeBytesSerialPort(i_frame, full_size);
            alarm(security_time);
            alarm_activated = FALSE;
            while (alarm_activated != TRUE && state != STOP)
            {
                if(readByteSerialPort(&b) < 0) perror("Connection error\n");
                if (state == START)
                {
                    if (b == FLAG)
                    {
                        state = FLAG_RCV;
                        printf("START->FLAG\n");
                    }
                    else
                    {
                        state = START;
                        printf("START->START\n");
                    }
                }
                else if (state == FLAG_RCV)
                {
                    if (b == FLAG)
                    {
                        state = FLAG_RCV;
                        printf("FLAG->FLAG\n");
                    }
                    else if (b == A_RECEIVER)
                    {
                        state = A_RCV;
                        printf("FLAG->A_RCV\n");
                    }
                    else
                    {
                        state = START;
                        printf("FLAG->START\n");
                    }
                }
                else if (state == A_RCV)
                {
                    if (b == FLAG)
                    {
                        state = FLAG_RCV;
                        printf("A_RCV->FLAG\n");
                    }
                    else if (b == C_UA)
                    {
                        state = C_RCV;
                        printf("A_RCV->C_RCV\n");
                    }
                    else
                    {
                        state = START;
                        printf("A_RCV->START\n");
                    }
                }
                else if (state == C_RCV)
                {
                    if ((b == C_REJ_0) || (b == C_REJ_1) )
                    {
                        transmission_state = FRAME_REJECTED;
                        state = FLAG_RCV;
                        printf("C_RCV->FLAG\n");
                    }
                    else if ((b == C_INF_0) || (b == C_INF_1))
                    {
                        transmission_state = FRAME_ACCEPTED;
                        Ns = N(Ns);
                        state = BCC_OK;
                        printf("C_RCV->BCC\n");
                    }
                    else
                    {
                        state = START;
                        printf("C_RCV->START\n");
                    }
                }
                else if (state == BCC_OK)
                {
                    if (b == FLAG)
                    {
                        state = STOP;
                        printf("ACABOU GG\n");
                    }
                    else
                    {
                        state = START;
                        printf("BCC->START\n");
                    }
                }
            }
            if ((transmission_state == FRAME_ACCEPTED) || (transmission_state == FRAME_REJECTED)) {
                break;
            }
            trys--;
        }
        if ((state != STOP) || (transmission_state == FRAME_REJECTED)) {
            return ERROR_OPENING;
        }
        else if (transmission_state == FRAME_ACCEPTED)
        {
            return full_size;
        }
        return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    int packet_ind = 0;
    unsigned char buf;
    unsigned char bcc2;
    while (trys != 0 && state != STOP)
        {
            alarm(security_time);
            alarm_activated = FALSE;
            while (alarm_activated != TRUE && state != STOP)
            {
                readByteSerialPort(&buf);
                if (state == START)
            {
                if (buf == FLAG)
                {
                    state = FLAG_RCV;
                    printf("START->FLAG\n");
                }
                else
                {
                    state = START;
                    printf("START->START\n");
                }
            }
            else if (state == FLAG_RCV)
            {
                if (buf == FLAG)
                {
                    state = FLAG_RCV;
                    printf("FLAG->FLAG\n");
                }
                else if (buf == A_SENDER)
                {
                    state = A_RCV;
                    printf("FLAG->A_RCV\n");
                }
                else
                {
                    state = START;
                    printf("FLAG->START\n");
                }
            }
            else if (state == A_RCV)
            {
                if (buf == FLAG)
                {
                    state = FLAG_RCV;
                    printf("A_RCV->FLAG\n");
                }
                else if (buf == C_SET)
                {
                    state = C_RCV;
                    printf("A_RCV->C_RCV\n");
                }
                else
                {
                    state = START;
                    printf("A_RCV->START\n");
                }
            }
            else if (state == C_RCV)
            {
                if (buf == FLAG)
                {
                    state = FLAG_RCV;
                    printf("C_RCV->FLAG\n");
                }
                else if (buf == (A_SENDER ^ C_SET))
                {
                    state = DATA;
                    printf("C_RCV->BCC\n");
                }
                else
                {
                    state = START;
                    printf("C_RCV->START\n");
                }
            }
            else if (state == DATA) {
                if (buf == ESCAPE) {
                    state = DATA_ESCPAPE_READ;
                }
                else if (buf == FLAG) {
                    state = DATA_FLAG_READ;
                }
                else {
                    packet[packet_ind] = buf;
                    packet_ind++;
                }
            }
        else if (state == DATA_ESCPAPE_READ) {
            if (buf == 0x5d) {
                packet[packet_ind] = ESCAPE;
                packet_ind++;
                state = DATA;
            }
            else if (buf == 0x5e) {
                packet[packet_ind] = FLAG;
                packet_ind++;
                state = DATA;
            }
        }
        else if (state == DATA_FLAG_READ) {
                    bcc2 = packet[0];
                    for (int i = 1; i < packet_ind - 1; i++) {
                        bcc2 = bcc2 ^ packet[i];
                    }
                    if (bcc2 == packet[packet_ind - 1]) {
                        packet[packet_ind] = '\0';
                        unsigned char c = Ns == 1 ? C_INF_1: C_INF_0;
                        state = STOP;
                        sendConnectionFrame(A_RECEIVER, c);
                        return packet_ind;
                    }
                    else {
                        unsigned char c = Ns == 1 ? C_REJ_1: C_REJ_0;
                        sendConnectionFrame(A_RECEIVER, c);
                        return -1;
                    }
        }
        }
        trys--;
        }
    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}
