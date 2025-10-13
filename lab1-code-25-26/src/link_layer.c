// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <unistd.h>

#define FLAG 0x7E
#define ADDR_ST_RR 0x03
#define ADDR_SR_RT 0x01
#define SET_CTRL 0x03
#define UA_CTRL 0x07

#define FALSE 0
#define TRUE 1
volatile int STOP = FALSE;

// Alarm Functionality
int alarmEnabled = FALSE;
int alarmCount = 0;

// State Machine States
typedef enum {
    start,
    flag_rcv,
    a_rcv,
    c_rcv,
    bcc_ok,
} states;

// SET and UA Bytes
unsigned char set_byte[5] = {FLAG, ADDR_ST_RR, SET_CTRL, ADDR_ST_RR^SET_CTRL, FLAG};
unsigned char ua_byte[5] = {FLAG, ADDR_ST_RR, UA_CTRL, ADDR_ST_RR^UA_CTRL, FLAG};

// Alarm Handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d received\n", alarmCount);
}

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{   
    int ret = openSerialPort(connectionParameters.serialPort,connectionParameters.baudRate);
    if(ret == -1) return EXIT_FAILURE;
    printf("Serial port opened.\n");

    int state;
    unsigned char address, control;

    /////////////////
    // TRANSMITTER //
    /////////////////

    if(connectionParameters.role == LlTx){

        // Write SET byte
        int bytes = writeBytesSerialPort(set_byte, 5);
        if(bytes == -1) return EXIT_FAILURE;
        printf("SET byte sent.\n");

        // Wait for it to arrive
        sleep(1);

        // Setup alarm
        struct sigaction act = {0};
        act.sa_handler = &alarmHandler;
        if (sigaction(SIGALRM, &act, NULL) == -1)
        {
            perror("sigaction");
            exit(1);
        }
        alarmEnabled = TRUE;
        alarm(3);
        printf("Alarm configured.\n");

        // Read UA byte
        while(STOP == FALSE && alarmCount < 4){

            unsigned char byte;
            int bytes = readByteSerialPort(&byte);
            if(bytes == -1) return EXIT_FAILURE;
            printf("Byte read: 0x%02X\n", byte);

            // If timeout resend SET byte
            if(alarmEnabled == FALSE){
                int ret = writeBytesSerialPort(set_byte, 5);
                if(ret == -1) return EXIT_FAILURE;
                printf("Timeout, resending SET byte.\n");

                alarmEnabled = TRUE;
                alarm(3);
                state = start;
                printf("Alarm configured again.\n");
            }

            // State machine for reading UA byte
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
                default:
                    if(byte == FLAG){
                        STOP = TRUE;
                        printf("UA byte read.\n");

                        // Disable alarm after UA byte is read
                        alarmEnabled = FALSE;
                        alarm(0);
                    } else {
                        state = start;
                    }
                break;
            }
        }

    //////////////
    // RECEIVER //
    //////////////
    } else {

        // Read SET byte
        while(STOP == FALSE){
            unsigned char byte;
            int bytes = readByteSerialPort(&byte);
            if(bytes == -1) return EXIT_FAILURE;
            printf("Byte read: 0x%02X\n", byte);

            // State machine for reading SET byte
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
                default:
                    if(byte == FLAG){
                        STOP = TRUE;

                        // SET byte read, send UA byte
                        int ret = writeBytesSerialPort(ua_byte, 5);
                        if(ret == -1) return EXIT_FAILURE;
                        printf("UA byte sent.\n");
                    } else {
                        state = start;
                    }
                break;
            }
        }
    }
    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO: Implement this function

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO: Implement this function

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose()
{
    // TODO: Implement this function

    return 0;
}
