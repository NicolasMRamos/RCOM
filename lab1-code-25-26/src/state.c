// State Machine (purely for reutilization purposes)

#include <stdlib.h>
#include "link_layer.h"

#define FLAG 0x7E
#define ADDR_ST_RR 0x03
#define ADDR_SR_RT 0x01
#define SET_CTRL 0x03
#define UA_CTRL 0x07

#define FALSE 0
#define TRUE 1
volatile int STOP = FALSE;

typedef enum {
    start,
    flag_rcv,
    a_rcv,
    c_rcv,
    bcc_ok,
} states;

int general_state_machine(){

    int state = 0;
    unsigned char address, control;

    unsigned char byte;
    int bytes = readByteSerialPort(&byte);
    if(bytes == -1) return EXIT_FAILURE;
    printf("Byte read: 0x%02X\n", byte);

    switch(state){
        case start:
            if(byte == FLAG){
                state = flag_rcv;
            } else {
                state = start;
            }
        break;
        case flag_rcv:
            if(byte == FLAG){
                state = flag_rcv;
            } else {
                address = byte;
                state = a_rcv;
            }
        break;
        case a_rcv:
            if(byte == FLAG){
                state = flag_rcv;
            } else {
                control = byte;
                state = c_rcv;
            }
        break;
        case c_rcv:
            if(byte == FLAG){
                state = flag_rcv;
            } else if ((address^control) == byte){
                state = bcc_ok;
            } else {
                state = start;
            }
        break;
        case bcc_ok:
            if(byte == FLAG){
                STOP = TRUE;
            } else {
                state = start;
            }
        break;
        default: break;
    }
}