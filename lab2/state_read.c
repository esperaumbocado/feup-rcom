#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP} message_state;
message_state state = START;

#define BUF_SIZE 5
#define FLAG 0x7E
#define A_R 0x03
#define C_R 0x03
#define BCC (A_R ^ C_R)

volatile int STOP = FALSE;



int main(int argc, char *argv[])
{
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
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
    newtio.c_cc[VTIME] = 0; 
    newtio.c_cc[VMIN] = 5;  

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    unsigned char buf[2] = {0}; 
    int count = 0;

    while (STOP == FALSE)
    {
        read(fd, buf, BUF_SIZE);
        
        // Check if a valid frame is received
        if (state == START) {
            if (buf[0] == FLAG) {
                state = FLAG_RCV;
            }
        }
        else if (state == FLAG_RCV) {
            if (buf[0] == FLAG_RCV) {
                state = FLAG_RCV;
            }
            else if (buf[0] == A_R) {
                state = A_RCV;
            }
            else {
                state = START
            }
        }
        else if (state == A_RCV) {
            
            
        }
        else if (state == C_RCV) {
            
        }
        else if (state == BCC_OK) {
            
        }
        else if (state == STOP) {
            
        }
    }

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
