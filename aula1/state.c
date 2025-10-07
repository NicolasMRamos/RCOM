#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BAUDRATE 38400
#define BUF_SIZE 256

#define FLAG 0x7E
#define A_WRITE 0x03
#define C_SET 0x03
#define BCC1 (A_WRITE^C_SET)
#define STOP_TEST 0x99


int fd = -1;           // File descriptor for open serial port
struct termios oldtio; // Serial port settings to restore on closing
volatile int STOP = FALSE;


int main(int argc, char *argv[]) {
    printf("Teste\n");

    unsigned char array[9] = {FLAG, A_WRITE, FLAG, BCC1, FLAG, A_WRITE, C_SET, BCC1, FLAG};
    int state = 0;
    for (int i = 0; i < sizeof(array); i++) {
        unsigned char byte = array[i];

        // read from serial port 
        // int byte = readByteSerialPort(&bytes);
        //while (state != 5) {
        printf("%d\n", byte);
        switch (byte) {
            case FLAG:
                if (state == 4) {
                    printf("FLAG - STATE 5\n");
                    state = 5;
                } else {        
                    printf("FLAG - STATE 1\n");
                    state = 1;
                }
                break;
            case A_WRITE:
                if (state == 1) {
                    printf("A - STATE 2\n");
                    state = 2;
                } else if (state == 2) {
                    printf("C - STATE 3\n");
                    state = 3;
                }
                else {
                    printf("A - STATE 0\n");
                    state = 0;
                }
                break;
            case BCC1:
                if (state == 3) {
                    printf("BCC1 - STATE 4\n");
                    state = 4;
                } else {
                    printf("BCC1 - STATE 0\n");
                    state = 0;
                }
                break;
            default:
                printf("OTHER_RCV - STATE 0\n");
                state = 0;
                break;
        }
       //}
    }

    return 0;
}
