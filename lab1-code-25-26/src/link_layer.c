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
#define ADDR_ST_RR 0x03
#define ADDR_SR_RT 0x01
#define SET_CTRL 0x03
#define UA_CTRL 0x07
#define C_DISC 0x0B

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
    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
unsigned char calculateBCC2(unsigned char *message, int sizeMessage) {
  unsigned char BCC2 = message[0];
  for (int i = 1; i < sizeMessage; i++) {
    BCC2 ^= message[i];
  }

  return BCC2;
}

int llwrite(const unsigned char *buf, int bufSize)
{
    unsigned char bcc2 = calculateBCC2(buf, bufSize);
    unsigned char stuffed_message[MAX_PAYLOAD_SIZE];

    // Init Stuffed Message
    stuffed_message[0] = FLAG;
    stuffed_message[1] = Adress;
    stuffed_message[2] = C;
    stuffed_message[3] = BCC1;

    // Byte Stuffing of Data
    int stuf_idx = 4;
    for (int idx = 0; idx < bufSize; idx++) {
        switch (buf[idx]) {
            case FLAG:
                stuffed_message[stuf_idx] = ESC;
                stuffed_message[stuf_idx + 1] = 0x5e;
                stuf_idx += 2;
                break;
            case ESC:
                stuffed_message[stuf_idx] = ESC;
                stuffed_message[stuf_idx + 1] = 0x5d;
                stuf_idx += 2;
                break;
            default:
                stuffed_message[stuf_idx] = buf[idx];
                stuf_idx++;
        }
    }

    // Byte Stuffing of BCC
    switch (bcc2) {
        case FLAG:
            stuffed_message[stuf_idx] = ESC;
            stuffed_message[stuf_idx + 1] = 0x5e;
            stuf_idx += 2;
            break;
        case ESC:
            stuffed_message[stuf_idx] = ESC;
            stuffed_message[stuf_idx + 1] = 0x5d;
            stuf_idx += 2;
            break;
        default:
            stuffed_message[stuf_idx] = buf[idx];
            stuf_idx++;
    }

    // Add the final flag
    stuffed_message[stuf_idx] = FLAG;

    // Send the message
    int return_code = writeBytesSerialPort(&stuffed_message, stuf_idx);

    // Alarme
    // Verificar se recebe RR ou REJ
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    unsigned char computed_bcc_2 = 0x00, received_bcc2;
    unsigned char prev_packet, curr_packet, address, control;
    unsigned char data[MAX_PAYLOAD_SIZE];
    int packet_index = 0;
    volatile int flag_received = FALSE, ignore_next = FALSE, first_data_byte = TRUE;
    int state = 0;
    int bytes;

    while(STOP == FALSE){
        bytes = readByteSerialPort(&curr_packet);
        if(bytes == -1) return EXIT_FAILURE;

        switch(state){ // Checks FLAG, Address, Control and BCC1
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
                    // TODO: esta mensagem vai ser usada para enviar para o write
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
int read_disc() {
    int bytes;
    unsigned char curr_packet;
    while (STOP == FALSE) {
        bytes = readByteSerialPort(&curr_packet);
        if(bytes == -1) return EXIT_FAILURE;

        // 
    }

    return 0;
}

int read_ua() {
    int bytes;
    unsigned char curr_packet;
    while (STOP == FALSE) {
        bytes = readByteSerialPort(&curr_packet);
        if(bytes == -1) return EXIT_FAILURE;

        // 
    }

    return 0;
}

int llclose()
{   
    unsigned char DISC_ST_RR[5] = {FLAG, ADDR_ST_RR, C_DISC, ADDR_ST_RR^C_DISC, FLAG};
    unsigned char DISC_SR_RT[5] = {FLAG, ADDR_SR_RT, C_DISC, ADDR_SR_RT^C_DISC, FLAG};
    unsigned char UA_ST_RR[5] = {FLAG, ADDR_ST_RR, UA_CTRL, ADDR_ST_RR^UA_CTRL, FLAG};

    // Transmitter
    if (connectionParameters.role == LlTx) {
        // write disc
        writeBytesSerialPort(&DISC_ST_RR, 5);

        // read disc
        read_disc();

        // write ua
        writeBytesSerialPort(&UA_ST_RR, 5);
        return 0;
    } 

    // Receiver
    if (connectionParameters.role == LlRx) {
        // read disc
        read_disc();

        // write disc
        writeBytesSerialPort(&DISC_SR_RT, 5);

        // read ua
        read_ua();

        return 0;
    }

    return  1;
}
