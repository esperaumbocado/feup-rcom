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
LinkLayerRole role;

int dataValid = 0;

// State machine related definitions
typedef enum {
    START, // Start state
    FLAG_RCV, // Received flag
    A_RCV, // Received A
    C_RCV, // Received C
    BCC_OK, // Received BCC
    STOP, // Stop state
    READING_DATA, // Reading data
    DATA_ESCAPE_RCV, // Received escape
    PENDING_DATA, // Pending data

} message_state; 

// State machine related variables
message_state state = START;


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


/*
SEND RR
*/
int sendRRFrame(int ns){
    unsigned char rr[5] = {FLAG, A_TX, ns == 0 ? C_RR0 : C_RR1, BCC1(A_TX, (ns == 0) ? C_RR0 : C_RR1), FLAG};
    printf(
        "====================\n"
        "Sending RR frame\n"
        "====================\n"
        );
    return writeBytesSerialPort(rr, 5);
}


/*
SEND REJ
*/
int sendREJFrame(int ns){
    unsigned char rej[5] = {FLAG, A_TX, ns == 0 ? C_REJ0 : C_REJ1, BCC1(A_TX, (ns == 0) ? C_REJ0 : C_REJ1), FLAG};
    printf(
        "====================\n"
        "Sending REJ frame\n"
        "====================\n"
        );
    return writeBytesSerialPort(rej, 5);
}

/*
SEND DISC
*/
int sendDISCFrame(){
    unsigned char disc[5] = {FLAG, A_TX, C_DISC, BCC1(A_TX, C_DISC), FLAG};
    printf(
        "====================\n"
        "Sending DISC frame\n"
        "====================\n"
        );
    return writeBytesSerialPort(disc, 5);
}

/*
TRANSMITTER STATE MACHINE
*/
void transmitterStateMachine(unsigned char byte){
    printf("Transmitter state machine\n");
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

        default:
            break;
    }

    logStateTransition((int)oldState, (int)state);  // Log the transition
}

/*
RECEIVER STATE MACHINE
*/
void receiverStateMachine(unsigned char byte){
    printf("Receiver state machine\n");
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

        default:
            break;
    }

    logStateTransition((int)oldState, (int)state);  // Log the transition
}


/*
TRANSMITTER DATA STATE MACHINE
Returns 1 if the STOP state
Returns 0 otherwise
dataValid is a pointer to a variable that will be set to 1 if the data is valid
*/
void dataResponseStateMachine(unsigned char byte, int* dataValid){
    message_state oldstate = state;
    printf("Data response state machine\n");
    
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
            *dataValid = 1;
        }else if (byte == (Ns == 0 ? C_REJ0 : C_REJ1)){
            state = C_RCV;
            *dataValid = 0;
        }else if (byte == FLAG){
            state = FLAG_RCV;
        }else{
            state = START;
        }
        break;
    case C_RCV:
        if (byte == C_RR0 || byte == C_RR1){
            state = BCC_OK;
        }else if (byte == C_REJ0 || byte == C_REJ1){
            state = FLAG_RCV;
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

    logStateTransition((int)oldstate, (int)state);
}


/*
Data Read State Machine
*/
int dataReadStateMachine(unsigned char byte, unsigned char *packet, int *packetIndex){
    printf("Data read state machine\n");
    printf("Byte received: 0x%02X\n", byte);
    message_state oldstate = state;
    int bcc2 = 0;

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
        if (byte == (Ns == 0 ? C_FRAME0 : C_FRAME1)){
            state = C_RCV;
        }else if (byte == FLAG){
            state = FLAG_RCV;
        }else if (byte == C_DISC){
            sendDISCFrame();
            return 0;
        }else{
            state = START;
        }
        break;
    case C_RCV:
        if (byte == BCC1(A_TX, (Nr == 0 ? C_FRAME0 : C_FRAME1))){
            state = READING_DATA;
        }else if (byte == FLAG){
            state = FLAG_RCV;
        }else{
            state = START;
        }
        break;
    case READING_DATA:
        if (byte == ESCAPE){
            state = DATA_ESCAPE_RCV;
        }else if (byte == FLAG){
            printf("Data flag received\n");
        
            printf("Test time\n");
            for (int i = 0; i < (*packetIndex - 1); i++){
                bcc2 ^= packet[i];
            }

            printf("End of test time\n");


            if (bcc2 == packet[*packetIndex - 1]){
                state = STOP;
                if (sendRRFrame(Nr) <= 0){
                    return -1;
                }
                packet[*packetIndex - 1] = '\0';
                Nr = (Nr + 1) % 2;
            }else{
                if (sendREJFrame(Nr)<= 0){
                    return -1;
                }
                Nr = (Nr + 1) % 2;
            }
        }else{
            packet[*packetIndex] = byte;
            (*packetIndex)++;
        }
        break;
    case DATA_ESCAPE_RCV:
        if (byte == 0x5E){
            packet[*packetIndex] = FLAG;
            (*packetIndex)++;
            state = READING_DATA;
        }else if (byte == 0x5D){
            packet[*packetIndex] = ESCAPE;
            (*packetIndex)++;
            state = READING_DATA;
        }else{
            packet[*packetIndex] = 0x7D;
            (*packetIndex)++;
            packet[*packetIndex] = byte;
            (*packetIndex)++;
            state = READING_DATA;
        }
        break;
    default:
        break;
    }

    logStateTransition((int)oldstate, (int)state);

    return 0;
}


/*
CLOSE STATE MACHINE
*/
void closeStateMachine(unsigned char byte,LinkLayerRole role){
    printf("Close state machine\n");
    message_state oldstate = state;

    switch (state)
    {
    case START:
        if (byte == FLAG){
            state = FLAG_RCV;
        }
        break;
    case FLAG_RCV:
        if (byte == A_TX && role == LlRx){
            state = A_RCV;
        }else if (byte == A_TX && role == LlTx){
            state = A_RCV;
        }else if (byte == FLAG){
            state = FLAG_RCV;
        }else{
            state = START;
        }
        break;
    case A_RCV:
        if (byte == C_DISC){
            state = C_RCV;
        }else if (byte == FLAG){
            state = FLAG_RCV;
        }else{
            state = START;
        }
        break;
    case C_RCV:
        if (byte == BCC1(A_TX, C_DISC) && role == LlRx){
            state = BCC_OK;
        }else if (byte == BCC1(A_TX, C_DISC) && role == LlTx){
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
    default:
        break;
    }

    logStateTransition((int)oldstate, (int)state);
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
    role = connectionParameters.role;

    signal(SIGALRM, handle_alarm);
    int retransmissionsLeft = nRetransmissions;

    unsigned char byte;

    switch(connectionParameters.role){
        case LlTx:
            alarmCount = 0;
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
                    }

                    if (state == STOP){
                        alarm(0);
                        return 0;
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
                }
            }
            sendUAFrame();
            break;
    }


    return 1;
}

/*
LLWRITE
If the frame is sent successfully, returns the number of bytes written
If the frame is not sent, returns 0
*/
int llwrite(const unsigned char *buf, int bufSize){
    
    int dataValid = 0;

    // Construction of the information frame
    int frameSize = bufSize + 6;
    unsigned char *i_frame = (unsigned char *)malloc(frameSize);
    i_frame[0] = FLAG;
    i_frame[1] = A_TX;
    i_frame[2] = (Ns == 0) ? C_FRAME0 : C_FRAME1;
    i_frame[3] = BCC1(A_TX, i_frame[2]);
    for (int i = 0; i < bufSize; i++){
        i_frame[i + 4] = buf[i];
    }
    
    unsigned char BCC2 = 0;
    for (int i = 0; i < bufSize; i++){
        BCC2 ^= buf[i];
    }
    
    int i = 4;
    for (int cur_byte = 0; cur_byte < bufSize; cur_byte++){
        if (buf[cur_byte] == FLAG || buf[cur_byte] == ESCAPE){
            frameSize++;
            i_frame = (unsigned char *)realloc(i_frame, frameSize);
            i_frame[i] = ESCAPE;
            i++;
            i_frame[i] = buf[cur_byte] ^ 0x20;
            printf("0x%02X\n", i_frame[4 + i]);
            i++;
        }else{
            i_frame[i] = buf[cur_byte];
            i++;
        }
    }

    i_frame[i] = BCC2;
    i++;
    i_frame[i] = FLAG;

    printf("Frame to be sent\n");
    for (int i = 0; i < frameSize; i++){
        printf("0x%02X ", i_frame[i]);
    }

    int retranmissionsLeft = nRetransmissions;

    alarmCount = 0;

    while (retranmissionsLeft > 0){
        if (writeBytesSerialPort(i_frame, frameSize) <= 0){
            printf("Error writing to serial port\n");
            return -1;
        }

        alarm(timeout);
        alarmEnabled = FALSE;

        unsigned char byte;

        printf ("====================\n"
                "Sent information frame\n"
                "====================\n");
        
        
        // Resetting state machine
        state = START;
        dataValid = 0;

        
        //Waiting for response
        while (!alarmEnabled){
                    if (readByteSerialPort(&byte) > 0){
                        dataResponseStateMachine(byte,&dataValid);
                    }else{
                        printf("Error reading from serial port\n");
                    }

                    if (state == STOP){
                        alarm(0);
                        break;
                    }
        }

        if (state == STOP && dataValid) {
            free(i_frame);
            Ns = (Ns + 1) % 2;
            printf("Receiver responded to the data as valid\n");
            return frameSize;
        }else if (state == STOP && !dataValid){
            free(i_frame);
            printf("Receiver responded to the data as invalid\n");
            return 0;
        }

        if (alarmEnabled){
            alarmEnabled = FALSE; 
            retranmissionsLeft--;  
            printf("Retransmission #%d\n", nRetransmissions - retranmissionsLeft);
        }

        if (retranmissionsLeft == 0){
            printf("Max retransmissions reached, giving up.\n");
            return -1;
        }
    }
    
    return 0;
}


////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet){
   
    unsigned char byte;
    int packetIndex = 0;
    state = START;

    while (state != STOP){
        if (readByteSerialPort(&byte) > 0){
            dataReadStateMachine(byte, packet, &packetIndex);
        }
    }
    
    
    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics){

    int retransmissionsLeft = nRetransmissions;
    unsigned char byte;

    switch(role){
        case LlTx:


            alarmCount = 0;

            // Try to send DISC frame with alarm until getting another DISC frame
            while (retransmissionsLeft > 0){
                if (sendDISCFrame() <= 0){
                    return -1;
                }

                alarm(timeout);
                alarmEnabled = FALSE;
                state = START;

                while (!alarmEnabled){
                    if (readByteSerialPort(&byte) > 0){
                        closeStateMachine(byte, role);
                    }

                    if (state == STOP){
                        sendUAFrame();
                        alarm(0);
                        return 0;
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
            unsigned char byte;
            // Wait for DISC frame, send DISC frame, wait for UA frame
            state = START;
            
            while (state != STOP){
                if (readByteSerialPort(&byte) > 0){
                    closeStateMachine(byte, role);
                }
            }

            if (state == STOP){
                // Try to send UA frame with alarm until getting a UA frame
                int retransmissionsLeft = nRetransmissions;

                alarmCount = 0;

                while (retransmissionsLeft > 0){
                    if (sendDISCFrame() <= 0){
                        return -1;
                    }


                    alarm(timeout);
                    alarmEnabled = FALSE;
                    state = START;

                    while (!alarmEnabled){
                        if (readByteSerialPort(&byte) > 0){
                            transmitterStateMachine(byte);
                        }

                        if (state == STOP){
                            alarm(0);
                            return 0;
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
            }

            break;
    }

    int clstat = closeSerialPort();
    return clstat;
}