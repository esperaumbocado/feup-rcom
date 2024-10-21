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

// More definitions
#define ESCAPE 0x7D

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

typedef enum {
    D_START, // Start state
    D_FLAG_RCV, // Received flag
    D_A_RCV, // Received A
    D_C_RCV, // Received C
    DATA_ESCPAPE_READ, // Read escape in data
    DATA_FLAG_READ, // Read flag in data
    DATA, // Its reading data
    D_STOP // Stop state
} data_packet_state; 

typedef enum {
    PACKET_REJECTED, // Packet was rejected
    PACKET_ACCEPTED, // Packet was accepted
    READING_PACKET, // Packet is beign read
} data_tranfer_state;

// State machine related variables
message_state state = START;
data_packet_state d_state = D_START;


// Numeration of data frames
int Ns = 0;
int Nr = 0;

// Debug external functions
extern void logStateTransition(int oldState, int newState);

/*
ALARM HANDLER
*/
void handle_alarm(int sig){
    alarmEnabled = TRUE;
    alarmCount++;
    printf("Alarm #%d\n", alarmCount); // Debug
}


/*
SEND SET
*/
int sendSetFrame(){
    unsigned char set[5] = {FLAG, A_TX, C_SET, BCC1(A_TX, C_SET), FLAG};
    printf(
        "====================\n"
        "Sending SET frame\n"
        "====================\n");
    return writeBytesSerialPort(set, 5);

}

/*
 SEND UA
*/
int sendUAFrame(){
    unsigned char ua[5] = {FLAG, A_TX, C_UA, BCC1(A_TX, C_UA), FLAG};
    printf(
        "====================\n"
        "Sending UA frame\n"
        "====================\n"
        );
    return writeBytesSerialPort(ua, 5);
}

int sendConnectionFrame(unsigned char A, unsigned char C) {
    unsigned char ua[5] = {FLAG, A, C, BCC1(A, C), FLAG};
    printf(
        "====================\n"
        "Sending Connection frame\n"
        "====================\n"
        );
    return writeBytesSerialPort(ua, 5);
}

/*
TRANSMITTER STATE MACHINE
*/
void transmitterStateMachine(unsigned char byte){
    message_state oldState = state;

    switch (state){
        case START:
            if (byte == FLAG){
                state = FLAG_RCV;
            }
            break;

        case FLAG_RCV:
            if (byte == A_TX){
                state = A_RCV;
            }else if (byte == FLAG){
                state = FLAG_RCV;
            }else{
                state = START;
            }
            break;

        case A_RCV:
            if (byte == C_UA){
                state = C_RCV;
            }else if (byte == FLAG){
                state = FLAG_RCV;
            }else{
                state = START;
            }
            break;

        case C_RCV:
            if (byte == BCC1(A_TX, C_UA)){
                state = BCC_OK;
            }else if (byte == FLAG){
                state = FLAG_RCV;
            }else{
                state = START;
            }
            break;

        case BCC_OK:
            if (byte == FLAG){
                state = STOP;
            }else{
                state = START;
            }
            break;

        case STOP:
            break;
    }

    logStateTransition((int)oldState, (int)state);  // Log the transition
}

/*
RECEIVER STATE MACHINE
*/
void receiverStateMachine(unsigned char byte){
    message_state oldState = state;

    switch (state)
    {
        case START:
            if (byte == FLAG){
                state = FLAG_RCV;
            }
            break;

        case FLAG_RCV:
            if (byte == A_TX){
                state = A_RCV;
            }else if (byte == FLAG){
                state = FLAG_RCV;
            }else {
                state = START;
            }
            break;

        case A_RCV:
            if (byte == C_SET){
                state = C_RCV;
            }else if (byte == FLAG){
                state = FLAG_RCV;
            }else{
                state = START;
            }
            break;

        case C_RCV:
            if (byte == BCC1(A_TX, C_SET)){
                state = BCC_OK;
            }else if (byte == FLAG){
                state = FLAG_RCV;
            }else{
                state = START;
            }
            break;

        case BCC_OK:
            if (byte == FLAG){
                state = STOP;
            }else{
                state = START;
            }
            break;

        case STOP:
            break;
    }

    logStateTransition((int)oldState, (int)state);  // Log the transition
}


/*
TRANSMITTER DATA STATE MACHINE
Returns 1 if the data frame is valid
Returns 0 if the data frame is invalid
*/
int dataResponseStateMachine(unsigned char byte){
    message_state oldstate = state;
    int valid = 0;

    switch (state)
    {
    case START:
        if (byte == FLAG){
            state = FLAG_RCV;
        }
        break;
    case FLAG_RCV:
        if (byte == A_TX){
            state = A_RCV;
        }else if (byte == FLAG){
            state = FLAG_RCV;
        }else{
            state = START;
        }
        break;
    case A_RCV:
        if (byte == (Ns == 0 ? C_RR0 : C_RR1)){
            state = C_RCV;
            valid = 1;
        }else if (byte == (Ns == 0 ? C_REJ0 : C_REJ1)){
            state = C_RCV;
            valid = 0;
        }else if (byte == FLAG){
            state = FLAG_RCV;
        }else{
            state = START;
        }
        break;
    case C_RCV:
        if (byte == BCC1(A_TX, (Ns == 0 ? C_RR0 : C_RR1)) || byte == BCC1(A_TX, (Ns == 0 ? C_REJ0 : C_REJ1))){
            state = BCC_OK;
        }else if (byte == FLAG){
            state = FLAG_RCV;
        }else{
            state = START;
        }
        break;
    case BCC_OK:
        if (byte == FLAG){
            state = STOP;
        }else{
            state = START;
        }
        break;
    
    default:
        break;
    }

    if (state == STOP && valid){
        return 1;
    }else{
        return 0;
    }
    
    logStateTransition((int)oldstate, (int)state);
}

void dataStateMachine(unsigned char byte){
    message_state oldstate = d_state;
    switch(d_state)
    {
        case D_START:
            if (byte == FLAG)
            {
                d_state = FLAG_RCV;
            }
            else
            {
                d_state = D_START;
            }
            break;
        case D_FLAG_RCV:
            if (byte == FLAG)
            {
                d_state = D_FLAG_RCV;
            }
            else if (byte == A_TX)
            {
                d_state = D_A_RCV;
            }
            else
            {
                d_state = D_START;
            }
            break;
        case D_A_RCV:
            if (byte == FLAG)
            {
                d_state = D_FLAG_RCV;
            }
            else if (byte == (Ns == 0 ? C_FRAME0: C_FRAME1))
            {
                d_state = D_C_RCV;
            }
            else
            {
                d_state = START;
            }
            break;
        case D_C_RCV:
            if (byte == FLAG)
            {
                d_state = D_FLAG_RCV;
            }
            else if (byte == BCC1(A_TX, (Ns == 0 ? C_FRAME0: C_FRAME1)))
            {
                d_state = DATA;
            }
            else
            {
                d_state = D_START;
            }
            break;
        case DATA:
            if (byte == ESCAPE) {
                d_state = DATA_ESCPAPE_READ;
            }
            else if (byte == FLAG) {
                d_state = DATA_FLAG_READ;
            }
            else {
                d_state = DATA;
            }
            break;
        case DATA_ESCPAPE_READ:
            d_state = DATA;
            break;
        case DATA_FLAG_READ: 
            d_state = D_STOP;
            break;
        case D_STOP:
            break;
    }
    logStateTransition((int)oldstate, (int)d_state);
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters){
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0){
        return -1;
    }

    nRetransmissions = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;

    signal(SIGALRM, handle_alarm);
    int retransmissionsLeft = nRetransmissions;

    unsigned char byte;

    switch(connectionParameters.role){
        case LlTx:
            while (retransmissionsLeft > 0){
                if (sendSetFrame() <= 0) {  // Send SET frame
                    return -1;
                }

                alarm(timeout);

                alarmEnabled = FALSE;
                state = START;  // Reset state machine

                // Wait for a response or timeout
                while (!alarmEnabled){
                    if (readByteSerialPort(&byte) > 0){
                        transmitterStateMachine(byte);  

                        if (state == STOP) {
                            alarm(0);
                            return 1;
                        }
                    }
                }

                if (alarmEnabled){
                    alarmEnabled = FALSE; 
                    retransmissionsLeft--;  
                    printf("Retransmission #%d\n", nRetransmissions - retransmissionsLeft);
                }
            }

            if (retransmissionsLeft == 0){
                printf("Max retransmissions reached, giving up.\n");
                return -1;
            }
            break;


        case LlRx:
            while (state != STOP){
                if (readByteSerialPort(&byte) > 0){
                    receiverStateMachine(byte);

                    if (state == STOP){
                        sendUAFrame();
                        return 1;
                    }
                }
            }
            break;
    }


    return 1;
}

/*
LLWRITE
If the frame is sent successfully, returns the number of bytes written
If the frame is not sent, returns 0
*/
int llwrite(const unsigned char *byte, int bufSize){
    
    int dataValid = 0;

    // Construction of the information frame

    int frameSize = bufSize + 6;
    unsigned char *i_frame = (unsigned char *)malloc(frameSize);
    i_frame[0] = FLAG;
    i_frame[1] = A_TX;
    i_frame[2] = (Ns == 0) ? C_FRAME0 : C_FRAME1;
    i_frame[3] = BCC1(A_TX, i_frame[2]);
    for (int i = 0; i < bufSize; i++){
        i_frame[i + 4] = byte[i];
    }
    
    unsigned char BCC2 = 0;
    for (int i = 0; i < bufSize; i++){
        BCC2 ^= byte[i];
    }
    
    // Stuffing the frame
    for (int cur_byte = 0; cur_byte < bufSize; cur_byte++){
        if (byte[cur_byte] == FLAG || byte[cur_byte] == ESCAPE){
            frameSize++;
            i_frame = (unsigned char *)realloc(i_frame, frameSize);
            i_frame[4 + cur_byte] = ESCAPE;
            cur_byte++;
            i_frame[4 + cur_byte] = byte[cur_byte - 1] ^ 0x20;
        }else{
            i_frame[4 + cur_byte] = byte[cur_byte];
        }
    }

    i_frame[frameSize - 2] = BCC2;
    i_frame[frameSize - 1] = FLAG;

    int retranmissionsLeft = nRetransmissions;

    while (retranmissionsLeft > 0)
    {
        if (writeBytesSerialPort(i_frame, frameSize) <= 0){
            printf("Error writing to serial port\n");
            return -1;
        }

        alarm(timeout);
        alarmEnabled = FALSE;

        state = START;

        printf ("====================\n"
                "Sent information frame\n"
                "====================\n");
        
        unsigned char byte;

        state = START;

        

        //Waiting for response
        while (!alarmEnabled){
                    if (readByteSerialPort(&byte) > 0){
                        dataResponseStateMachine(byte);  
                        Ns = (Ns + 1) % 2;
                        if (state == STOP) {
                            alarm(0);
                            return 1;
                        }
                    }

                    if (alarmEnabled){
                        alarm(timeout);
                        alarmEnabled = FALSE; 
                        retranmissionsLeft--;  
                        printf("Retransmission #%d\n", nRetransmissions - retranmissionsLeft);
                    }
        }

        if (retranmissionsLeft == 0){
            printf("Max retransmissions reached, giving up.\n");
            return -1;
        }
    }
    
    free(i_frame);
    if (dataValid){
        return frameSize;
    }
    return 0;
}


////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    unsigned char buf;
    int packet_ind = 0;
    unsigned char bcc2;
    while (d_state != D_STOP)
    {
        if (readByteSerialPort(&buf) == 1) {
            printf("byte = 0x%02X\n", buf);
            dataStateMachine(buf);
            if (d_state == DATA_FLAG_READ) {
                bcc2 = packet[0];
                for (int i = 1; i < packet_ind - 1; i++) {
                    bcc2 = bcc2 ^ packet[i];
                }
                if (bcc2 == packet[packet_ind - 1]) {
                    packet[packet_ind] = '\0';
                    unsigned char c = Ns == 1 ? C_RR1: C_RR0;
                    sendConnectionFrame(A_RX, c);
                    return packet_ind;
                }
                else {
                    unsigned char c = Ns == 1 ? C_REJ1: C_REJ0;
                    sendConnectionFrame(A_RX, c);
                    return -1;
                }
            }
            else if (d_state == DATA_ESCPAPE_READ) {
                if (buf == 0x5d) {
                    packet[packet_ind - 1] = ESCAPE;
                }
                else if (buf == 0x5e) {
                    packet[packet_ind - 1] = FLAG;
                }
            }
            else {
                packet[packet_ind] = buf;
                packet_ind++;
            }
        }
    }
    return 0;
}
////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics){
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}