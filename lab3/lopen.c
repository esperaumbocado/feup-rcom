
#include "link_layer.h"
#include "serial_port.h"
#include "definitions.h"
#include <termios.h>
#include <unistd.h>

#define _POSIX_SOURCE 1 
#define FLAG 0x7E
#define A_SENDER 0X03
#define A_RECEIVER 0X01
#define BUF_SIZE 5
#define C_SET 0x03
#define C_UA 0x07
#define FALSE 0
#define TRUE 1
#define ERROR_OPENING -1

typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP} message_state; 
message_state state = START;

int alarm_cycle = 0;
int alarm_activated = FALSE;
int trys = 0;
int security_time = 0;


void alarmHandler(int signal) {
    alarm_activated = TRUE;
    alarm_cycle++;
}

int llopen(LinkLayer connectionParameters)
{
    const char *serialPort = connectionParameters.serialPort;
    int fd = open(serialPort, O_WRONLY | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPort);
        return ERROR_OPENING;
    }

    struct termios oldtio;
    struct termios newtio;

    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 30;
    newtio.c_cc[VMIN] = 5;
    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }
    
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0)
    {
        return -1;
    }

    switch (connectionParameters.role)
    {
    case LlTx:
        (void)signal(SIGALRM, alarmHandler);
        unsigned char set_frame[BUF_SIZE] = {FLAG, A_SENDER, C_SET, A_SENDER ^ C_SET, FLAG};
        int bytes = write(fd, set_frame, BUF_SIZE);
        trys = connectionParameters.nRetransmissions;
        security_time = connectionParameters.timeout;

        while (trys != 0 && state != STOP)
        {
            alarm(security_time);
            alarm_activated = FALSE;
            while (alarm_activated != TRUE && state != STOP)
            {
                buf[0] = 0x00;
                read(fd, buf, 1);
                printf("F = 0x%02X\n", buf[0]);
                if (state == START)
                {
                    if (buf[0] == FLAG)
                    {
                        state = FLAG_RCV;
                        printf("START->FLAG\n");
                    }
                    else
                    {
                        state = START;
                        printf("START->START\n");
                  0  }
                }
                else if (state == FLAG_RCV)
                {
                    if (buf[0] == FLAG)
                    {
                        state = FLAG_RCV;
                        printf("FLAG->FLAG\n");
                    }
                    else if (buf[0] == A_RECEIVER)
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
                    if (buf[0] == FLAG)
                    {
                        state = FLAG_RCV;
                        printf("A_RCV->FLAG\n");
                    }
                    else if (buf[0] == C_UA)
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
                    if (buf[0] == FLAG)
                    {
                        state = FLAG_RCV;
                        printf("C_RCV->FLAG\n");
                    }
                    else if (buf[0] == (A_RECEIVER ^ C_UA))
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
                    if (buf[0] == FLAG)
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
        }
        break;

    case LlRx:
        unsigned char frame[BUF_SIZE] = {0};
        read(fd, frame, 1);
        printf("F = 0x%02X\n", buf[0]);
        
        while (state != STOP)
        {
            if (state == START)
            {
                if (buf[0] == FLAG)
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
                if (buf[0] == FLAG)
                {
                    state = FLAG_RCV;
                    printf("FLAG->FLAG\n");
                }
                else if (buf[0] == A_SENDER)
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
                if (buf[0] == FLAG)
                {
                    state = FLAG_RCV;
                    printf("A_RCV->FLAG\n");
                }
                else if (buf[0] == C_SET)
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
                if (buf[0] == FLAG)
                {
                    state = FLAG_RCV;
                    printf("C_RCV->FLAG\n");
                }
                else if (buf[0] == (A_SENDER ^ C_SET))
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
                if (buf[0] == FLAG)
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
        break;
    }
    unsigned char ua_frame[BUF_SIZE] = {FLAG, A_SENDER, C_SET, A_SENDER ^ C_SET, FLAG};
    int bytes = write(fd, ua_frame, BUF_SIZE);
    return fd;
}