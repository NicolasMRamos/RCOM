// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

    

    ret = llclose();
    if (ret == -1) {
        printf("Error: llclose failed\n");
        exit(EXIT_FAILURE);
    }
}
