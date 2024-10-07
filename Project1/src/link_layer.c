
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
#define FALSE 0
#define TRUE 1
#define I_FRAME_SIZE 6
#define ERROR_OPENING -1

typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DATA, STOP} message_state; 
typedef enum {FRAME_ACCEPTED, FRAME_REJECTED} t_state;
message_state state = START;
t_state transmission_state = FRAME_REJECTED;

int alarm_cycle = 0;
int alarm_activated = FALSE;
int trys = 0;
int security_time = 0;
int Ns = 0;
int Nr = 1;


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
    int full_size = I_FRAME_SIZE + bufSize;
    unsigned char i_frame[full_size];
    i_frame[SYNC_BYTE_1] = FLAG;
    i_frame[ADRESS_FIELD] = A_SENDER;
    i_frame[CONTROL_FIELD] = N(Ns);
    i_frame[HEADER_PROTECTION_FIELD] = i_frame[ADRESS_FIELD] ^ i_frame[CONTROL_FIELD];
    unsigned char BCC2 = buf[0];
    i_frame[DATA_FIELD] = buf[0];
    for (int buf_byte = 1; i < buf_byte, buf_byte++) {
        i_frame[DATA_FIELD + buf_byte] = buf[buf_byte];
        BCC2 = BCC2 ^ buf[buf_byte];
    }
    i_frame[DATA_PROTECTION_FIELD + bufSize] = BCC2;
    i_frame[SYNC_BYTE_2 + bufSize] = FLAG;
    while (trys != 0 && transmission_state != FRAME_ACCEPTED)
    {
        alarm(security_time);
        alarm_activated = FALSE;
        while (alarm_activated != TRUE && state != STOP)
        {
            writeBytesSerialPort(i_frame, full_size);
        }
        break;
    }
    unsigned char ua_frame[BUF_SIZE] = {FLAG, A_SENDER, C_SET, A_SENDER ^ C_SET, FLAG};
    int bytes = writeBytesSerialPort(ua_frame, BUF_SIZE);
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
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
