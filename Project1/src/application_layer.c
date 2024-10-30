// Application layer protocol implementation

#include "application_layer.h"
#include <math.h>

#define CONTROL_FIELD_DATA 2
#define CONTROL_FIELD_START 1
#define CONTROL_FIELD_END 3
#define TYPE_FILE_SIZE 0
#define TYPE_FILE_NAME 1

#define MAX_PACKET_SIZE 1000

char num_bits(int n){
    char k = 0;
    while(n){
        n >>= 1;
        k++;
    }
    return k;
}

unsigned char* buildControlPacket(int control_field, int L1, int length, int L2, const char* filename);
unsigned char* buildDataPacket(unsigned char sequence_number, unsigned char* packet_data, int data_size);


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer link_layer_info;
    strcpy(link_layer_info.serialPort, serialPort);
    link_layer_info.baudRate = baudRate;
    link_layer_info.role = strcmp(role, "tx") ? LlRx : LlTx;
    link_layer_info.nRetransmissions = nTries;
    link_layer_info.timeout = timeout;
    int fd = llopen(link_layer_info);
    if (fd < 0){
        printf(
            "====================================\n"
            "Error opening link layer connection\n"
            "====================================\n");
    }else{
        printf(
            "====================================\n"
            "Link layer connection established\n"
            "fd = %d\n"
            "====================================\n",
            fd);
    }
    
    switch (link_layer_info.role) {
        case LlTx:
            {
            FILE* tx_file = fopen(filename, "rb");
            long file_size = 0;
            int sequence = 0;
            struct stat st;
            stat(filename, &st);
            printf("filename %s \n", filename);
            file_size = st.st_size;
            printf("file size %li \n", file_size);
            int L1 = num_bits(file_size)/8 + 1;
            int L2 = strlen(filename);
            unsigned char* control_packet_start = buildControlPacket(CONTROL_FIELD_START, L1, file_size, L2, filename);
            int bufSize = 5 + L1 + L2;
            printf("sending control");
            if (llwrite((const unsigned char*)control_packet_start, bufSize) == -1) {
                printf("Error starting\n");
                exit(-1);
            }
            long bytes_left = file_size;
            printf("control established \n");
            free(control_packet_start);
            while (bytes_left != 0) {
                printf("bytes left = %li", bytes_left);
                long data_size = bytes_left > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : bytes_left;
                unsigned char* data_to_send = (unsigned char*)malloc(sizeof(unsigned char) * data_size);
                fread(data_to_send, sizeof(unsigned char), data_size, tx_file);
                unsigned char* data_packet = buildDataPacket(sequence, data_to_send, data_size);
                int llwrite_return = llwrite(data_packet, data_size + 4);
                if(llwrite_return == -1) {
                    printf("===========================\n"
                           "Error sending data packet\n"
                           "STOPPING TRANSMISSION\n"
                           "===========================\n");
                    exit(-1);
                } else if (llwrite_return == 0) {
                    printf("Retransmission\n");
                    fseek(tx_file, -data_size, SEEK_CUR); // Move the file pointer back to retry reading the same part
                    continue;
                } else {
                    bytes_left = bytes_left - data_size;
                    sequence = (sequence + 1) % 99;
                }
            }
            printf("file sent \n");
            unsigned char * control_packet_end = buildControlPacket(CONTROL_FIELD_END, L1, file_size, L2, filename);
            if (llwrite((const unsigned char*)control_packet_end, bufSize) == -1) {
                printf("Error starting\n");
                exit(-1);
            }
            printf("close \n");
            fclose(tx_file);
            free(control_packet_end);
            break;
            }

        case LlRx:
            {
            unsigned char * packet = (unsigned char *) malloc(MAX_PACKET_SIZE);
            llread(packet);
            int bytes_recieved = 0;
            unsigned char size_legth_bytes = packet[2];
            unsigned char new_file_size = 0;
            printf("Size length bytes %d \n", size_legth_bytes);
            for(int i = 0; i < size_legth_bytes; i++) {
                new_file_size |= packet[3 + i] << (size_legth_bytes - i - 1) * 8;
            }
            printf("new file size %d \n", new_file_size);
            
            FILE* reciever_file = fopen((const char*)filename, "wb+");
            if (reciever_file == NULL) {
                printf("Error opening file \n");
                exit(-1);
            }

            int num = 1;
            while(packet[0] != CONTROL_FIELD_END) {
                printf("PACKER NUMBER %d \n", num);
                int packet_recived_size = llread(packet);
                if (packet_recived_size == -1) {
                    printf("INVALID PACKET \n");
                    continue;
                }
                if (packet[0] != CONTROL_FIELD_END) {
                    printf("packet recieved size %d \n", packet_recived_size);
                    bytes_recieved += MAX_PACKET_SIZE;
                    fwrite(packet+4, sizeof(unsigned char), packet_recived_size - 4, reciever_file);
                    printf("bytes recieved %d \n", bytes_recieved);
                }else{
                    printf("END PACKET RECIEVED \n");
                    //fclose(reciever_file);  // CAUSES free() invalid size
                    printf("file recieved \n");
                    break;
                }
            }
            //fclose(reciever_file);
            printf("close reciever \n");
            break;
            }
    }
    llclose(0);
}

// Build the data packet to be sent
unsigned char* buildDataPacket(unsigned char sequence_number, unsigned char* packet_data, int data_size) { 
    unsigned char* packet = (unsigned char*) malloc(4 + data_size);
    packet[0] = CONTROL_FIELD_DATA;
    packet[1] = sequence_number;
    packet[2] = (data_size >> 8) & 0xFF, 
    packet[3] = data_size & 0xFF;
    memcpy(packet+4, packet_data, data_size);
    return packet;
}

//Build the control packet
unsigned char* buildControlPacket(int control_field, int L1, int length, int L2, const char* filename) {
    int ind = 0;
    unsigned char* packet = (unsigned char*) malloc(5 + L1 + L2);
    packet[ind] = control_field;
    ind++;
    packet[ind] = TYPE_FILE_SIZE;
    ind++;
    packet[ind] = L1;
    ind++;
    for(int i = 0; i < L1; i++) {
        packet[ind] = (length >> 8 * (L1 - i - 1)) && 0xFF;
        ind++;
    }
    packet[ind] = TYPE_FILE_NAME;
    ind++;
    packet[ind] = L2;
    ind++;
    for(int i = 0; i < L2; i++) {
        packet[ind] = filename[i];
        ind++;
    }
    return packet;
}

