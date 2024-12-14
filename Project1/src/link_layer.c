// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

// GENERAL DEFINITIONS
#define FALSE 0
#define TRUE 1

// PACKET RELATED DEFINITIONS
#define FLAG 0x7E

#define A_TX 0x03
#define A_RX 0x01

// Supervision and Unnumbered frame related definitions
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
t_state transmission_state = FRAME_REJECTED;

// Numeration of data frames
int Ns = 0;
int Nr = 1;

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
// LLCLOSE
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
                    packet[packet_ind] = buf;
                    packet_ind++;
                    packet[packet_ind] = 0x5d;
                    packet_ind++;
                }
                else if (buf == FLAG) {
                    packet[packet_ind] = buf;
                    packet_ind++;
                    packet[packet_ind] = 0x5e;
                    packet_ind++;
                    if (bcc2 == packet[packet_ind - 1]) {
                        state = STOP;
                        return packet_ind;
                    }
                    else {
                        unsigned char c = Ns == 1 ? C_REJ_1: C_REJ_0;
                        sendConnectionFrame(A_RECEIVER, c);
                        return -1;
                    }
                }
                else {
                    bcc2 = (packet_ind == 0) ? packet : bcc2 ^ packet;
                    packet[packet_ind] = buf;
                    packet_ind+;
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