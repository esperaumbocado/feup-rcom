
#include "link_layer.h"
#include "serial_port.h"
#include <termios.h>
#include <unistd.h>

#define _POSIX_SOURCE 1 

typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP} message_state; message_state state = START;

int alarm_cycle = 0;
bool alarm_activated = FALSE;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
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
        return -1;
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
        unsigned char feedback_buf[5] = {0};
        
    
    while (STOP == FALSE)
    {
        if (alarmCount > 2)
        {
            printf("Alarm count exceeded\n");
            break;
        }

        if (alarmEnabled == FALSE)
        {
            alarm(3); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }

        printf("Waiting for frame\n");
        int bytes = read(fd, feedback_buf, BUF_SIZE);
        printf("Frame received\n");
        buf[bytes] = '\0'; 


        // Checking UA WORD
        if (feedback_buf[0] == FLAG &&
            feedback_buf[1] == A_SENDER &&
            feedback_buf[2] == C_UA &&
            feedback_buf[3] == (feedback_buf[1]^feedback_buf[2]) &&
            feedback_buf[4] == FLAG){
                printf("====================================\n");
                printf("Correct UA WORD received.\n");
                printf("FLAG = 0x%02X\n", feedback_buf[0]);
                printf("A = 0x%02X\n", feedback_buf[1]);
                printf("C = 0x%02X\n", feedback_buf[2]);
                printf("BCC = 0x%02X\n", ((feedback_buf[2])^(feedback_buf[3])));
                printf("====================================\n");
                STOP = TRUE;
                alarm(0);
                alarmEnabled = FALSE;
            }
        


        break;
    }
    case LlRx:
        
        break;
    }

    // TODO

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

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
