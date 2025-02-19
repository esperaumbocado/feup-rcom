#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <termios.h> 
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is // included by <termios.h> #define BAUDRATE B9600 #define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0 
#define TRUE 1

typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP} message_state; message_state state = START;

#define BUF_SIZE 2 
#define FLAG 0x7E 
#define A_R 0x03 
#define C_R 0x03 
#define BCC (A_R ^ C_R)

int main(int argc, char *argv[]) { const char *serialPortName = argv[1];

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
newtio.c_cc[VMIN] = 1;  

tcflush(fd, TCIOFLUSH);

if (tcsetattr(fd, TCSANOW, &newtio) == -1)
{
    perror("tcsetattr");
    exit(-1);
}

printf("New termios structure set\n");

unsigned char buf[2] = {0};     

while (state != STOP)
{
    read(fd, buf, 1);
    printf("F = 0x%02X\n", buf[0]);        
    if (state == START) {
        if (buf[0] == FLAG) {
            state = FLAG_RCV;
            printf("START->FLAG\n");
        }
        else {
            state = START;
            printf("START->START\n"); 
        }
    }
    else if (state == FLAG_RCV) {
        if (buf[0] == FLAG) {
            state = FLAG_RCV;
            printf("FLAG->FLAG\n");
        }
        else if (buf[0] == A_R) {
            state = A_RCV;
            printf("FLAG->A_RCV\n");
        }
        else {
            state = START;
            printf("FLAG->START\n");
        }
    }
    else if (state == A_RCV) {
        if (buf[0] == FLAG) {
            state = FLAG_RCV;
            printf("A_RCV->FLAG\n");
        }
        else if (buf[0] == C_R) {
            state = C_RCV;    
            printf("A_RCV->C_RCV\n");           
        }
        else {
            state = START;
            printf("A_RCV->START\n");
        }
    }
    else if (state == C_RCV) {
        if (buf[0] == FLAG) {
            state = FLAG_RCV;
            printf("C_RCV->FLAG\n");
        }
        else if (buf[0] == BCC) {
            state = BCC_OK;
            printf("C_RCV->BCC\n");
        }
        else {
            state = START;
            printf("C_RCV->START\n");
        }
    }
    else if (state == BCC_OK) {
       if (buf[0] == FLAG) {
            state = STOP;
            printf("ACABOU GG\n");
        }
        else {
            state = START;
            printf("BCC->START\n");
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