// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "link_layer.h"

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

    unsigned char buffer1[5] = {0x01,0x02,0x03,0x04, 0x05};
    unsigned char buffer2[5] = {0x06,0x07,0x08,0x09, 0x10};

    if(configs.role == LlTx){
        ret = llwrite(buffer1, 5);
        if (ret == -1) {
            printf("Error: llwrite failed\n");
            exit(EXIT_FAILURE);
        }
        ret = llwrite(buffer2, 5);
        if (ret == -1) {
            printf("Error: llwrite failed\n");
            exit(EXIT_FAILURE);
        }
    } else {
        unsigned char packet[MAX_PAYLOAD_SIZE];
        ret = llread(packet);
        if (ret == -1) {
            printf("Error: llread failed\n");
            exit(EXIT_FAILURE);
        }
        ret = llread(packet);
        if (ret == -1) {
            printf("Error: llread failed\n");
            exit(EXIT_FAILURE);
        }
    }

    ret = llclose();
    if (ret == -1) {
        printf("Error: llclose failed\n");
        exit(EXIT_FAILURE);
    }
}
