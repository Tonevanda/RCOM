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

typedef struct
{
    int nOfTimeouts;

    int nOfBytesllopenSent;
    int nOfPacketsllopenSent;
    int nOfBytesllopenReceived;
    int nOfPacketsllopenReceived; 

    int nOfBytesllwriteSent;
    int nOfPacketsllwriteSent;
    int nOfBytesllwriteReceived;
    int nOfPacketsllwriteReceived;
    int nOfCharStuffed;

    int nOfBytesllreadSent;
    int nOfPacketsllreadSent;
    int nOfBytesllreadReceived;
    int nOfPacketsllreadReceived;
    int nOfREJSent;
    int nOfCharDestuffed;

    int nOfBytesllcloseSent;
    int nOfPacketsllcloseSent;
    int nOfBytesllcloseReceived;
    int nOfPacketsllcloseReceived;
} Statistics;

extern Statistics statistics;

extern int v;

unsigned char* createControlPacket(unsigned int c, int filesize, const char* filename,int* packSize){

    int decimalNumber = filesize;
    char hexadecimalString[10];
    sprintf(hexadecimalString, "%x", decimalNumber);
    int count = 0;
    while(hexadecimalString[count]!='\0'){
        count++;
    }
    int L1 = (int) ceilf((float)count/2.0);
    int L2 = strlen(filename);
    *packSize = 5 + L1 + L2;

    unsigned char *packet = (unsigned char*)malloc(*packSize);

    packet[0] = c;
    packet[1] = 0;
    packet[2] = L1;
    
    for (unsigned char i = 0 ; i < L1 ; i++) {
        packet[2+L1-i] = filesize & 0xFF;
        filesize = filesize >> 8;
    }
    
    packet[2+L1+1] = 1;
    packet[2+L1+2] = L2;
    int offset = 2 + L1 + 1 + 1 + 1; // need the offset of the V2 field for the memcpy
    memcpy(packet + offset, filename, L2);
    return packet;
}

unsigned char* createDataPacket(unsigned char *data, int dataSize, int *packetSize){

    *packetSize = 1 + 2 + dataSize;

    unsigned char* packet = (unsigned char*)malloc(*packetSize);

    packet[0] = 1;
    packet[1] = (dataSize >> 8) & 0xFF;
    packet[2] = dataSize & 0xFF;
    memcpy(packet + 3, data, dataSize);

    return packet;
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename){

    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort,serialPort);
    connectionParameters.role = strcmp(role, "tx") ? LlRx : LlTx;
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    int fd = llopen(connectionParameters);
    if (fd < 0) {
        perror("Connection error\n");
        exit(-1);
    }
    switch (connectionParameters.role) {
        //write
        case LlTx: {
            FILE* file = fopen(filename, "rb");
            if (file == NULL) {
                perror("File not found\n");
                exit(-1);
            }    
            fseek(file, 0, SEEK_END);
            int fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);
            int controlPacketSize=0;
            unsigned char *controlPacketStart = createControlPacket(2,fileSize,filename,& controlPacketSize);
            llwrite(controlPacketStart,controlPacketSize);

            unsigned char* content = (unsigned char*)malloc(sizeof(unsigned char) * fileSize);
            fread(content, sizeof(unsigned char), fileSize, file);

            int dataPacketSize=0;
            int bytesSent=fileSize;
            while (bytesSent >= 0) { 

                int dataSize = bytesSent > (long int) MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : bytesSent;
                unsigned char* data = (unsigned char*) malloc(dataSize);
                memcpy(data, content, dataSize);
                
                unsigned char *dataPacketStart = createDataPacket(data,dataSize,&dataPacketSize);

                llwrite(dataPacketStart,dataPacketSize);
                bytesSent -= (long int) MAX_PAYLOAD_SIZE; 
                content += dataSize;  
            }

            unsigned char *controlPacketEnd = createControlPacket(3,fileSize,filename,& controlPacketSize);
            llwrite(controlPacketEnd,controlPacketSize);

            break;
        }

        //read
        case LlRx: {
            int bufSize=0;
            unsigned char* output = (unsigned char*) malloc((MAX_PAYLOAD_SIZE*2)+8);
            bufSize = llread(output);
            v=0;
            for (int i = 0; i<output[2]; i++) {
                v |= (long)(output[3+i] << 8*(output[2]-1-i));
            }
            unsigned char* fileoutput = (unsigned char*) malloc(v);

            char* originalFileName = (char*) malloc(output[2+output[2]+2]);
            char* fileExtension = (char*) malloc(5);
            int isFileExtension=FALSE;
            int extentionCharCounter=0;
            int j=0;
            int i;
            for(i = 2 + output[2] + 3 ; i<2 + output[2] + 3 + output[2+output[2]+2];i++){
                if(output[i]=='.'){
                    isFileExtension=TRUE;
                }
                if(isFileExtension){
                    extentionCharCounter++;
                    fileExtension[j++]=output[i];
                }
                else{
                    originalFileName[i-2 - output[2] - 3]=output[i];
                }
            }
            i-=2 + output[2] + 3 + 4;
            originalFileName[i++]='-';
            originalFileName[i++]='r';
            originalFileName[i++]='e';
            originalFileName[i++]='c';
            originalFileName[i++]='e';
            originalFileName[i++]='i';
            originalFileName[i++]='v';
            originalFileName[i++]='e';
            originalFileName[i++]='d';
            memcpy(originalFileName+i,fileExtension,extentionCharCounter);
            int totalsize=0;
            while(bufSize!=-2){
                bufSize = llread(output);
                if(output[0]==1){
                    memcpy(fileoutput+totalsize,output+3,bufSize-3);
                    totalsize+=(bufSize-3);
                }
            }
            //stat fix
            statistics.nOfBytesllreadReceived-=5;
            statistics.nOfBytesllcloseReceived+=5;
            statistics.nOfPacketsllcloseReceived++;
            
            printf("Final open\n");
            FILE *fp = fopen( originalFileName , "wb" );
            if (fp==NULL) {
                perror("Connection error\n");
            }
            fwrite(fileoutput, sizeof(unsigned char), totalsize, fp);
            fclose(fp);
            break;
        }
    }
    if(llclose(1)){
        printf("Closing failure\n");
        exit(-1);
    }
}
