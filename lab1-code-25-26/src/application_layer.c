// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define START 0x01
#define END 0x03
#define DATA_PKT_CTRL 0x02
#define CTRL_PKT_SIZE 7

#define FALSE 0
#define TRUE 1

// Auxiliary functions definitions (implementations in the end of the file)
int buildControlPacket(unsigned char *ctrl_packet, const long fileSize, const char *filename, int *ctrl_packetSize, int is_start);
int buildDataPacket(unsigned char *data_packet, const int bufferSize, const unsigned char *buffer);
int parseControlPacket(const unsigned char *ctrl_packet, char *filename, long *fileSize);

// Main
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    int ret;

    LinkLayer configs;
    strcpy(configs.serialPort, serialPort);
    configs.baudRate = baudRate;
    configs.nRetransmissions = nTries;
    configs.timeout = timeout;

    if(strcmp(role,"tx") == 0) configs.role = LlTx;
    else configs.role = LlRx;

    ret = llopen(configs);
    if (ret == -1) {
        printf("Error: llopen failed\n");
        exit(EXIT_FAILURE);
    }


    if(configs.role == LlTx){

        // Open file
        FILE* file = fopen(filename, "rb");
        if(file == NULL){
            printf("Error: couldn't open file\n");
            exit(EXIT_FAILURE);
        }

        // Acquire file size
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        rewind(file);

        // Build START control packet
        unsigned char start_ctrl_packet[MAX_PAYLOAD_SIZE];
        int ST_ctrlPktSize;

        buildControlPacket(start_ctrl_packet, fileSize, filename, &ST_ctrlPktSize, TRUE);

        // Send START control packet
        ret = llwrite(start_ctrl_packet, ST_ctrlPktSize);
        if(ret == -1){
            printf("Error: llwrite failed\n");
            exit(EXIT_FAILURE);
        } else {
            printf("START Frame Size: %d\n", ret);
        }

        // Data packet preparation
        unsigned char data_packet[2 * MAX_PAYLOAD_SIZE + 6];

        unsigned char buffer[MAX_PAYLOAD_SIZE];
        int bytesRead;

        // Send data packet
        while ((bytesRead = fread(buffer, 1, MAX_PAYLOAD_SIZE, file)) > 0){

            int data_packetSize = buildDataPacket(data_packet, bytesRead, buffer);

            ret = llwrite(data_packet, data_packetSize);
            if(ret == -1){
                printf("Error: llwrite failed\n");
                exit(EXIT_FAILURE);
            } else {
                printf("Data Frame Size: %d\n", ret);
            }

        }

        // Build END control packet
        unsigned char end_ctrl_packet[MAX_PAYLOAD_SIZE];
        int END_ctrlPktSize;

        buildControlPacket(end_ctrl_packet, fileSize, filename, &END_ctrlPktSize, FALSE);

        // Send END control packet
        ret = llwrite(end_ctrl_packet, END_ctrlPktSize);
        if(ret == -1){
            printf("Error: llwrite failed\n");
            exit(EXIT_FAILURE);
        } else {
            printf("END Frame Size: %d\n", ret);
        }

        fclose(file);

    } else {

        // Prepare to read data packet
        unsigned char packet[MAX_PAYLOAD_SIZE+3];
        int packetSize;

        // Read data packet
        packetSize = llread(packet);
        if(packetSize == -1){
            printf("Error: llread failed\n");
            exit(EXIT_FAILURE);
        }

        // Acquire file configs
        char newFileName[256];
        long newFileSize = 0;
        parseControlPacket(packet, newFileName, &newFileSize);

        // Open new file to write on
        FILE *file = fopen(filename, "wb");
        if(file == NULL){
            printf("Error: file failed to open\n");
            exit(EXIT_FAILURE);
        }

        int totalWritten = 0;

        // Write to file while there are bytes to read
        while(1) {

            packetSize = llread(packet);
            if(packetSize == -1){
                printf("Error: llread failed\n");
                exit(EXIT_FAILURE);
            }

            if(packet[0] == END){ // Received END control packet
                break; 
            }

            if(packet[0] == DATA_PKT_CTRL){ // Received data packet
                int dataLength = (packet[1] << 8) | packet[2];
                printf("Data length: %d\n", dataLength);
                fwrite(&packet[3], 1, dataLength, file);
                totalWritten += dataLength;
            }
        }

        fclose(file);

        if(totalWritten != newFileSize){
            printf("Incorrect number of bytes read!\n");
        } else {
            printf("Correct number of bytes read!\n");
        }

    }

    ret = llclose();
    if (ret == -1) {
        printf("Error: llclose failed\n");
        exit(EXIT_FAILURE);
    }
}

// Auxiliary functions implementations

int buildControlPacket(unsigned char *ctrl_packet, const long fileSize, const char *filename, int *ctrl_packetSize, int is_start){

    // Define control for START or END packet
    if(is_start){
        ctrl_packet[0] = START;
    } else {
        ctrl_packet[0] = END;
    }

    // File size
    ctrl_packet[1] = 0x00;
    ctrl_packet[2] = 0x04; // long, 4 bytes
    // File size dividing
    ctrl_packet[3] = (fileSize >> 24) & 0xFF;
    ctrl_packet[4] = (fileSize >> 16) & 0xFF;
    ctrl_packet[5] = (fileSize >> 8) & 0xFF;
    ctrl_packet[6] = fileSize & 0xFF;

    // File name
    int filenameLength = strlen(filename);
    ctrl_packet[7] = 0x01;
    ctrl_packet[8] = filenameLength;
    memcpy(&ctrl_packet[9], filename, filenameLength);

    // Control packet size
    *ctrl_packetSize = 9 + filenameLength;

    return 0;
}

int buildDataPacket(unsigned char *data_packet, const int bufferSize, const unsigned char *buffer){

    // Data packet control
    data_packet[0] = DATA_PKT_CTRL;

    // Bit shifting and masking to obtain L1 and L2
    unsigned char L2 = (bufferSize >> 8) & 0xFF;
    unsigned char L1 = bufferSize & 0xFF;

    data_packet[1] = L2;
    data_packet[2] = L1;

    // Data packet pointer
    int i = 3;

    // Packet building
    for(int j = 0; j < bufferSize; j++){
        data_packet[i] = buffer[j];
        i++;
    }

    // Return correct data packet size
    return bufferSize + 3;
}

int parseControlPacket(const unsigned char *ctrl_packet, char *filename, long *fileSize){

    // Reconstruct file size
    *fileSize = ((long)ctrl_packet[3] << 24) | ((long)ctrl_packet[4] << 16) | ((long)ctrl_packet[5] << 8)  | ((long)ctrl_packet[6]);

    // Reconstruct file name
    int nameLength = ctrl_packet[8];
    for (int i = 0; i < nameLength; i++) {
        filename[i] = ctrl_packet[9 + i];
    }

    // Null terminate string
    filename[nameLength] = '\0';

    return 0;
}