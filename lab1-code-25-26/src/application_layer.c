// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <stdio.h>
#include<string.h>


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer configs;
    strcpy(configs.serialPort, serialPort);
    configs.baudRate = baudRate;
    configs.nRetransmissions = nTries;
    configs.timeout = timeout;
    if(strcmp(role, "tx") == 0) configs.role = LlTx;
    else configs.role = LlRx;   
    llopen(configs);
}
