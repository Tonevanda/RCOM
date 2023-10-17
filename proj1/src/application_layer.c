// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort,serialPort);
    connectionParameters.role = strcmp(role, "tx") ? LlRx : LlTx;
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    llopen(connectionParameters);

}

unsigned char* createControlPacket(unsigned int c, int filesize, char* filename){
    int L1 = (int) log2f((float)(filesize/8));
    int L2 = strlen(filename);
    int size = 5 + L1 + L2;

    unsigned char *packet = (unsigned char*)malloc(size);

    packet[0] = c;
    packet[1] = 0;
    packet[2] = L1;

    for (unsigned char i = 0 ; i < L1 ; i++) {
        packet[2+L1-i] = filesize & 0xFF;
        filesize = filesize >> 8;
    }

    packet[2+L1] = 1;
    packet[2+L1+1] = L2;
    memcpy(packet[2+L1+1+1], filename, L2);
    return packet;
}
