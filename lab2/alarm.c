#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#define FALSE 0
#define TRUE 1
#define BAUDRATE B38400
#define _POSIX_SOURCE 1

int alarmEnabled = FALSE;
int alarmCount = 0;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <SerialPort>\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(argv[1]);
        return -1;
    }

    struct termios oldtio, newtio;

    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        return -1;
    }

    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 30;  // 3 second timeout (30 * 0.1s)
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    (void)signal(SIGALRM, alarmHandler);

    unsigned char frame[5] = {0x7E, 0x03, 0x03, 0x00, 0x7E};
    frame[3] = frame[1] ^ frame[2];  // Calculate BCC

    printf("Sending frame...\n");
    alarm(3);
    alarmEnabled = TRUE;

    while (alarmCount < 3)
    {

        if (alarmEnabled == FALSE )
        {
            write(fd, frame, 5);
            alarm(3);
            alarmEnabled = TRUE;
            unsigned char buf2[5 + 1];
            sleep(1);
            read(fd, buf2, 5);
            if ((buf2[0] == 0x7E) && (buf2[1] == 0x01) && (buf2[2] == 0x07) && (buf2[3] == (buf2[1] ^ buf2[2])) && (buf2[4] == 0x7E)) {
            printf("Match");
            printf("var = 0x%02X\n", buf2[0]);
            printf("var = 0x%02X\n", buf2[1]);
            printf("var = 0x%02X\n", buf2[2]);
            printf("var = 0x%02X\n", buf2[3]);
            printf("var = 0x%02X\n", buf2[4]);
        }
        }
    }

    close(fd);
    return 0;
}