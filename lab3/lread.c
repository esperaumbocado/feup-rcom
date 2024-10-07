
#include "link_layer.h"
#include "serial_port.h"
#include "definitions.h"
#include <termios.h>
#include <unistd.h>

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
#define ERROR_RETURN -1

typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP} message_state; 
typedef enum {FRAME_ACCEPTED, FRAME_REJECTED} t_state;
message_state state = START;

int alarm_cycle = 0;
int alarm_activated = FALSE;
int trys = 0;
int current_inf_frame = 0;
int security_time = 0;
t_state transmission_state = FRAME_REJECTED;

int N(int current_frame) {
    return (current_frame + 1) % 2;
}

void alarmHandler(int signal) {
    alarm_activated = TRUE;
    alarm_cycle++;
}

int llread(unsigned char *packet)
{
    unsigned char i_frame[I_FRAME_SIZE + bufSize];
    i_frame[SYNC_BYTE_1] = FLAG;
    i_frame[ADRESS_FIELD] = A_SENDER;
    i_frame[CONTROL_FIELD] = N(current_inf_frame);
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
                
        }
        break;
    }
    unsigned char ua_frame[BUF_SIZE] = {FLAG, A_SENDER, C_SET, A_SENDER ^ C_SET, FLAG};
    int bytes = write(fd, ua_frame, BUF_SIZE);
    return fd;
}