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
#define C_RR0 0xAA
#define C_RR1 0xAB
#define C_REJ0 0x54
#define C_REJ1 0x55
#define C_DISC 0x0B

// BCC related definitions
#define BCC1(A, C) (A ^ C)

// Information frame related definitions
#define C_FRAME0 0x00
#define C_FRAME1 0x80

// Alarm related
int alarmEnabled = FALSE;
int alarmCount = 0;

// Retransmission related
int nRetransmissions = 0;
int timeout = 0;

// State machine related definitions
typedef enum {
    START, // Start state
    FLAG_RCV, // Received flag
    A_RCV, // Received A
    C_RCV, // Received C
    BCC_OK, // Received BCC
    STOP // Stop state
} message_state; 

// State machine related variables
message_state state = START;


// Debug external functions
extern void logStateTransition(int oldState, int newState);

////////////////////////////////////////////////
// ALARM HANDLER
////////////////////////////////////////////////
void handle_alarm(int sig)
{
    alarmEnabled = TRUE;
    alarmCount++;
    printf("Alarm #%d\n", alarmCount); // Debug
}


////////////////////////////////////////////////
// SEND SET
////////////////////////////////////////////////
int sendSetFrame()
{
    unsigned char set[5] = {FLAG, A_TX, C_SET, BCC1(A_TX, C_SET), FLAG};
    printf(
        "====================\n"
        "Sending SET frame\n"
        "====================\n"
        );
    return writeBytesSerialPort(set, 5);
}

////////////////////////////////////////////////
// SEND UA
////////////////////////////////////////////////
int sendUAFrame()
{
    unsigned char ua[5] = {FLAG, A_TX, C_UA, BCC1(A_TX, C_UA), FLAG};
    printf(
        "====================\n"
        "Sending UA frame\n"
        "====================\n"
        );
    return writeBytesSerialPort(ua, 5);
}


////////////////////////////////////////////////
// TRANSMITTER STATE MACHINE
////////////////////////////////////////////////
void transmitterStateMachine(unsigned char byte)
{
    message_state oldState = state;

    switch (state)
    {
        case START:
            if (byte == FLAG)
            {
                state = FLAG_RCV;
            }
            break;

        case FLAG_RCV:
            if (byte == A_TX)
            {
                state = A_RCV;
            }
            else if (byte == FLAG)
            {
                state = FLAG_RCV;
            }
            else
            {
                state = START;
            }
            break;

        case A_RCV:
            if (byte == C_UA)
            {
                state = C_RCV;
            }
            else if (byte == FLAG)
            {
                state = FLAG_RCV;
            }
            else
            {
                state = START;
            }
            break;

        case C_RCV:
            if (byte == BCC1(A_TX, C_UA))
            {
                state = BCC_OK;
            }
            else if (byte == FLAG)
            {
                state = FLAG_RCV;
            }
            else
            {
                state = START;
            }
            break;

        case BCC_OK:
            if (byte == FLAG)
            {
                state = STOP;
            }
            else
            {
                state = START;
            }
            break;

        case STOP:
            break;
    }

    logStateTransition((int)oldState, (int)state);  // Log the transition
}

////////////////////////////////////////////////
// RECEIVER STATE MACHINE
////////////////////////////////////////////////
void receiverStateMachine(unsigned char byte)
{
    message_state oldState = state;

    switch (state)
    {
        case START:
            if (byte == FLAG)
            {
                state = FLAG_RCV;
            }
            break;

        case FLAG_RCV:
            if (byte == A_TX)
            {
                state = A_RCV;
            }
            else if (byte == FLAG)
            {
                state = FLAG_RCV;
            }
            else 
            {
                state = START;
            }
            break;

        case A_RCV:
            if (byte == C_SET)
            {
                state = C_RCV;
            }
            else if (byte == FLAG)
            {
                state = FLAG_RCV;
            }
            else
            {
                state = START;
            }
            break;

        case C_RCV:
            if (byte == BCC1(A_TX, C_SET))
            {
                state = BCC_OK;
            }
            else if (byte == FLAG)
            {
                state = FLAG_RCV;
            }
            else
            {
                state = START;
            }
            break;

        case BCC_OK:
            if (byte == FLAG)
            {
                state = STOP;
            }
            else
            {
                state = START;
            }
            break;

        case STOP:
            break;
    }

    logStateTransition((int)oldState, (int)state);  // Log the transition
}


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0)
    {
        return -1;
    }

    nRetransmissions = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;

    signal(SIGALRM, handle_alarm);
    int retransmissionsLeft = nRetransmissions;

    unsigned char byte;

    switch(connectionParameters.role)
    {
        case LlTx:
            while (retransmissionsLeft > 0)
            {
                if (sendSetFrame() <= 0)  // Send SET frame
                {
                    return -1;
                }

                alarm(timeout);

                alarmEnabled = FALSE;
                state = START;  // Reset state machine

                // Wait for a response or timeout
                while (!alarmEnabled)
                {
                    if (readByteSerialPort(&byte) > 0)
                    {
                        transmitterStateMachine(byte);  

                        if (state == STOP) 
                        {
                            alarm(0);
                            return 1;
                        }
                    }
                }

                if (alarmEnabled)
                {
                    alarmEnabled = FALSE; 
                    retransmissionsLeft--;  
                    printf("Retransmission #%d\n", nRetransmissions - retransmissionsLeft);
                }
            }

            if (retransmissionsLeft == 0)
            {
                printf("Max retransmissions reached, giving up.\n");
                return -1;
            }
            break;


        case LlRx:

            while (state != STOP)
            {
                if (readByteSerialPort(&byte) > 0)
                {
                    receiverStateMachine(byte);

                    if (state == STOP)
                    {
                        sendUAFrame();
                        return 1;
                    }
                }
            }
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