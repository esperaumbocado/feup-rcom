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
#define BAUDRATE B9600
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 5
#define FLAG 0x7E

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

    unsigned char buf[BUF_SIZE + 1] = {0}; 

    while (STOP == FALSE)
    {
        printf("Waiting for frame\n");
        read(fd, buf, BUF_SIZE);
        printf("Frame received\n");
        
        // Check if a valid frame is received
        if ((buf[0] == 0x7E) && (buf[1] == 0x03) && (buf[2] == 0x03) && (buf[3] == (buf[1] ^ buf[2])) && (buf[4] == 0x7E)) {
            printf("====================================\n");
            printf("Correct SET WORD received.\n");
            printf("FLAG = 0x%02X\n", buf[0]);
            printf("A = 0x%02X\n", buf[1]);
            printf("C = 0x%02X\n", buf[2]);
            printf("BCC = 0x%02X\n", buf[3]);
            printf("====================================\n");
            unsigned char buf2[BUF_SIZE] = {
                0x7E,        // Flag (start)
                0x03,        // Address field
                0x07,        // Control field (UA command, for example)
                0x00,        // BCC (Address XOR Control)
                0x7E         // Flag (end)
            };
            buf2[3] = buf2[1] ^ buf2[2];
            printf("Sending UA frame\n");
            int bytes = write(fd, buf2, BUF_SIZE);
            printf("%d bytes written\n", bytes);
            printf("====================================\n");
            if (bytes < 0) {
               perror("write");
               exit(-1);
            }
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
