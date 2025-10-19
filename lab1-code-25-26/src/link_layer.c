// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

// Byte Camps
#define ESC 0x7D
#define FLAG 0x7E
#define ESC_toinsert 0x5D
#define FLAG_toinsert 0x5E
#define ADDR_ST_RR 0x03
#define ADDR_SR_RT 0x01
#define SET_CTRL 0x03
#define UA_CTRL 0x07
#define DISC_CTRL 0x0B

#define I0C 0x00
#define I1C 0x40

// RR and REJ signals
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81

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

// Supervisory Frames
unsigned char rr0_byte[5] = {FLAG, ADDR_ST_RR, RR0, ADDR_ST_RR^RR0, FLAG};
unsigned char rr1_byte[5] = {FLAG, ADDR_ST_RR, RR1, ADDR_ST_RR^RR1, FLAG};
unsigned char rej0_byte[5] = {FLAG, ADDR_ST_RR, REJ0, ADDR_ST_RR^REJ0, FLAG};
unsigned char rej1_byte[5] = {FLAG, ADDR_ST_RR, REJ1, ADDR_ST_RR^REJ1, FLAG};

// Information Frame Identification
int control_num = 0;
unsigned char info_control = 0x00;

// DISC Byte
unsigned char disc_byte[5] = {FLAG, ADDR_ST_RR, DISC_CTRL, ADDR_ST_RR^DISC_CTRL, FLAG};

// Role Saving
LinkLayerRole role;

// Alarm Handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
}

// Functionality and MISC
#define FALSE 0
#define TRUE 1
volatile int STOP = FALSE;

#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{   
    int ret = openSerialPort(connectionParameters.serialPort,connectionParameters.baudRate);
    if(ret == -1) return EXIT_FAILURE;
    printf("Serial port opened.\n");

    // Role Saving
    role = connectionParameters.role;

    int state = start;
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
                case bcc_ok:
                    if(byte == FLAG){
                        printf("UA byte read. Disabling alarm.\n");

                        // Disable alarm after UA byte is read
                        alarmEnabled = FALSE;
                        alarm(0);

                        STOP = TRUE;
                    } else {
                        state = start;
                    }
                break;
                default: break;
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
                case bcc_ok:
                    if(byte == FLAG){
                        printf("SET byte read, sending UA byte..\n");
                        
                        // SET byte read, send UA byte
                        int ret = writeBytesSerialPort(ua_byte, 5);
                        if(ret == -1) return EXIT_FAILURE;
                        printf("UA byte sent.\n");

                        STOP = TRUE;
                    } else {
                        state = start;
                    }
                break;
                default: break;
            }
        }
    }
    printf("Connection established, returning now.\n");
    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    unsigned char bcc2 = compute_bcc2(buf, bufSize);
    unsigned char frame[MAX_PAYLOAD_SIZE];
    int index = 4, frameSize, bytes;

    // Header building
    frame[0] = FLAG;
    frame[1] = ADDR_ST_RR;
    frame[2] = info_control;
    frame[3] = info_control^ADDR_ST_RR;

    // Payload building
    for(int i = 0; i < bufSize; i++){
        if(buf[i] == ESC){
            frame[index] = ESC;
            frame[index+1] = ESC_toinsert;
            index += 2;
        } else if (buf[i] == FLAG){
            frame[index] = ESC;
            frame[index+1] = FLAG_toinsert;
            index += 2;
        } else {
            frame[index] = buf[i];
            index++;
        }
    }

    // Trailer building
    frame[index] = bcc2;
    index++;
    frame[index] = FLAG;
    frameSize = index;

    // Write frame
    bytes = writeBytesSerialPort(frame,frameSize);
    if(bytes == -1) return EXIT_FAILURE;
    printf("Information Frame %d sent.\n", control_num);

    // Wait for packet to arrive
    sleep(1);

    // Prepare for reading answer
    int state = 0;
    unsigned char address, control;
    unsigned char byte;

    // Alarm setup
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

    volatile int rej_received = FALSE;

    // Reading answer
    while(STOP == FALSE && alarmCount < 4){

        bytes = readByteSerialPort(&byte);
        if(bytes == -1) return EXIT_FAILURE;
        printf("Byte read: 0x%02X\n", byte);

        // Alarm/Rejection check
        if(alarmEnabled == FALSE){
            int ret = writeBytesSerialPort(frame, frameSize);
            if(ret == -1) return EXIT_FAILURE;
            printf("Timeout, resending frame.\n");

            alarmEnabled = TRUE;
            alarm(3);
            state = start;
            printf("Alarm configured again.\n");
        } else if (rej_received == TRUE){
            int ret = writeBytesSerialPort(frame, frameSize);
            if(ret == -1) return EXIT_FAILURE;
            printf("REJ received, resending frame.\n");

            rej_received = FALSE;
            alarmEnabled = TRUE;
            alarm(3);
            state = start;
            printf("Alarm restarted.\n");
        }

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
                    if(control == RR0){
                        control_num = 0;
                        info_control = 0x00;
                        printf("RR0 received.\n");
                        STOP = TRUE;
                    } else if(control == RR1){
                        control_num = 1;
                        info_control = 0x40;
                        printf("RR1 received.\n");
                        STOP = TRUE;
                    } else if(control == REJ0){
                        printf("REJ0 received, resending information packet 0.\n");
                        rej_received = TRUE;
                        STOP = FALSE;
                    } else {
                        printf("REJ1 received, resending information packet 1.\n");
                        rej_received = TRUE;
                        STOP = FALSE;
                    }
                } else {
                    state = start;
                }
            break;
            default: break;
        }

    }

    return frameSize;
}

// BCC2 computation through frame payload
unsigned char compute_bcc2(const unsigned char *buf, int bufSize){
    unsigned char bcc2 = 0x00;
    for(int i = 0; i < bufSize; i++){
        bcc2 ^= buf[i];
    }
    return bcc2;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // Prepare for reading frame
    unsigned char computed_bcc_2 = 0x00, received_bcc2;
    unsigned char prev_packet, curr_packet, address, control;
    unsigned char data[MAX_PAYLOAD_SIZE];
    int packet_index = 0;
    volatile int flag_received = FALSE, ignore_next = FALSE, first_data_byte = TRUE;
    int state = 0;
    int bytes;

    // Reading frame
    while(STOP == FALSE){
        bytes = readByteSerialPort(&curr_packet);
        if(bytes == -1) return EXIT_FAILURE;
        printf("Byte read: 0x%02X\n", curr_packet);

        switch(state){
            case start:
                if(curr_packet == FLAG){
                    state = flag_rcv;
                    printf("Flag Received.\n");
                } else {
                    state = start;
                }
            break;
            case flag_rcv:
                if(curr_packet == FLAG){
                    state = flag_rcv;
                    printf("Flag Received.\n");
                } else {
                    address = curr_packet;
                    state = a_rcv;
                    printf("Address Received.\n");
                }
            break;
            case a_rcv:
                if(curr_packet == FLAG){
                    state = flag_rcv;
                    printf("Flag Received.\n");
                } else {
                    control = curr_packet;
                    state = c_rcv;
                    printf("Control Received.\n");
                }
            break;
            case c_rcv:
                if(curr_packet == FLAG){
                    state = flag_rcv;
                    printf("Flag Received.\n");
                } else if ((address^control) == curr_packet){
                    state = bcc_ok;
                    printf("BCC1 OK, starting payload analysis.\n");
                } else {
                    if(control == I0C){
                        bytes = writeBytesSerialPort(rej0_byte, 5);
                        if(bytes == -1) return EXIT_FAILURE;
                    } else {
                        bytes = writeBytesSerialPort(rej1_byte, 5);
                        if(bytes == -1) return EXIT_FAILURE;
                    }
                    printf("BCC1 wrong, sending REJ.\n");
                }
            break;
            case bcc_ok:
                if(curr_packet == ESC){
                    ignore_next = TRUE;
                    printf("Received ESC! Skipping..\n");
                    break;
                }
                if(ignore_next == TRUE){
                    ignore_next = FALSE;
                    print("Previous data was ESC, disregarding current data meaning.\n");
                } else if(curr_packet == FLAG && ignore_next == FALSE){
                    state = 5;
                    received_bcc2 = prev_packet;
                    printf("Flag received! Checking BCC2..\n");
                    break;
                }

                if(!first_data_byte){ // This is so prev_packet isn't analyzed while undefined 
                    data[packet_index] = prev_packet;
                    packet_index++;
                    computed_bcc_2 ^= prev_packet;
                }

                prev_packet = curr_packet;
                first_data_byte = FALSE;
            break;
            default: 
                if(received_bcc2 == computed_bcc_2){
                    if(control == I0C){
                        bytes = writeBytesSerialPort(rr1_byte, 5);
                        if(bytes == -1) return EXIT_FAILURE;
                    } else {
                        bytes = writeBytesSerialPort(rr0_byte, 5);
                        if(bytes == -1) return EXIT_FAILURE;
                    }
                    printf("BCC2 OK! Sending RR.\n");
                } else {
                    if(control == I0C){
                        bytes = writeBytesSerialPort(rej0_byte, 5);
                        if(bytes == -1) return EXIT_FAILURE;
                    } else {
                        bytes = writeBytesSerialPort(rej1_byte, 5);
                        if(bytes == -1) return EXIT_FAILURE;
                    }
                    printf("BCC2 wrong, sending REJ.\n");
                }
                STOP = TRUE;
            break;
        }
    }

    if (packet == NULL || data == NULL || packet_index > MAX_PAYLOAD_SIZE) {
        printf("memcpy error: invalid buffer\n");
        return EXIT_FAILURE;
    }
    memcpy(packet, data, packet_index);

    printf("Packet contains correct payload, returning..\n");

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose()
{
    int bytes;

    if(role == LlTx){

        // Send DISC
        bytes = writeBytesSerialPort(disc_byte, 5);
        if(bytes == -1) return EXIT_FAILURE;

        // Prepare for reading
        int state = 0;
        unsigned char byte;
        unsigned char address, control;

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

        // Reading DISC byte
        while(STOP == FALSE){

            bytes = readByteSerialPort(&byte);
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
                        printf("DISC byte read, sending UA byte..\n");
                        
                        // DISC byte read, send UA byte
                        int ret = writeBytesSerialPort(ua_byte, 5);
                        if(ret == -1) return EXIT_FAILURE;
                        printf("UA byte sent.\n");

                        STOP = TRUE;
                    } else {
                        state = start;
                    }
                break;
                default: break;
            }
        }

    } else {

        // Prepare for reading
        int state = 0;
        unsigned char byte;
        unsigned char address, control;

        // Reading DISC byte
        while(STOP == FALSE){

            bytes = readByteSerialPort(&byte);
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
                        printf("DISC byte read, sending DISC byte..\n");
                        
                        // DISC byte read, send DISC back
                        int ret = writeBytesSerialPort(disc_byte, 5);
                        if(ret == -1) return EXIT_FAILURE;
                        printf("DISC byte sent.\n");

                        STOP = TRUE;
                    } else {
                        state = start;
                    }
                break;
                default: break;
            }
        }

    }

    return 0;
}
