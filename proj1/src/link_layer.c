// Link layer protocol implementation

#include "link_layer.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>


// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define _POSIX_SOURCE 1 // POSIX compliant source

#define BUF_SIZE 256

#define FLAG 0x7E
#define SET 0x03
#define UA 0x07
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81
#define DISC 0x0B
#define FRAME0 0x00
#define FRAME1 0x40

#define T_PROP 0.1
#define PROBABILITY 0

typedef enum{
    FIRSTFLAG,      //0
    A,              //1
    C,              //2
    BCC1,           //3
    DATA,           //4
    BCC2,           //5
    FINALFLAG,      //6
    SUCCESS,        //7
    FAILURE,        //8
    DISCONNECTING   //9
} State;

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

volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmTimeout = 0;
int nRetransmissions = 0;
unsigned char frame=0x00;
int fd;
LinkLayer connectionParam;
Statistics statistics;
struct timeval begin, end;
int v;
struct termios oldtio;
struct termios newtio;
int filePort;


speed_t getBaudRate(int baud){
    switch (baud) {
        case 110:
            return B110;
        case 300:
            return B300;
        case 600:
            return B600;
        case 1200:
            return B1200;
        case 2400:
            return B2400;
        case 4800:
            return B4800;
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        case 230400:
            return B230400;
        case 460800:
            return B460800;
        case 500000:
            return B500000;
        case 576000:
            return B576000;
        case 921600:
            return B921600;
        case 1000000:
            return B1000000;
        case 1152000:
            return B1152000;
        case 1500000:
            return B1500000;
        case 2000000:
            return B2000000;
        case 2500000:
            return B2500000;
        case 3000000:
            return B3000000;
        case 3500000:
            return B3500000;
        case 4000000:
            return B4000000;
        default: 
            return -1;
    }
}

int connect(const char *serialPort){
    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    filePort = open(serialPort, O_RDWR | O_NOCTTY);
    printf("Connected\n");
    if (filePort < 0)
    {
        perror(serialPort);
        exit(-1);
    }


    // Save current port settings
    if (tcgetattr(filePort, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = getBaudRate(connectionParam.baudRate) | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0.1; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)
    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by file but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(filePort, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(filePort, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    return filePort;
}

void alarmHandler(int signal){
    alarmEnabled = FALSE;
    nRetransmissions--;
    statistics.nOfTimeouts++;
    STOP=TRUE;
    printf("Alarm #%d\n", nRetransmissions);
}

int writeSupervisionFrame(unsigned char A, unsigned char C){
    unsigned char supervisionFrame[5] = {FLAG, A, C, A ^ C, FLAG};
    int bytesWritten = 0;
    if((bytesWritten = write(fd, supervisionFrame, 5)) < 0) return -1;
    sleep(1);
    return bytesWritten;
}

int stuffBytes(unsigned char *buf_write, int *bufSize, const unsigned char *buf){
    buf_write[0] = FLAG;
    buf_write[1] = 0x03;
    buf_write[2] = frame;
    buf_write[3] = buf_write[1] ^ buf_write[2];
    int offset=4;
    unsigned char bcc2Field=0x00;
    for(int i=0;i<*bufSize;i++){
        if(buf[i]==0x7E){
            buf_write[i+offset]=0x7D;
            offset++;
            buf_write[i+offset]=0x5E;
            statistics.nOfCharStuffed++;
        }
        else if (buf[i]==0x7D)
        {
            buf_write[i+offset]=0x7D;
            offset++;
            buf_write[i+offset]=0x5D;
            statistics.nOfCharStuffed++;
        }
        else{
            buf_write[i+offset]=buf[i];
        } 
        bcc2Field^=buf[i];
    }
    if(bcc2Field==0x7E){
        buf_write[*bufSize+offset] = 0x7D;
        offset++;
        buf_write[*bufSize+offset] = 0x5E;
        statistics.nOfCharStuffed++;
    }
    else if (bcc2Field==0x7D)
    {
        buf_write[*bufSize+offset] = 0x7D;
        offset++;
        buf_write[*bufSize+offset] = 0x5D;
        statistics.nOfCharStuffed++;
    }
    else{
        buf_write[*bufSize+offset] = bcc2Field;
    }
    offset++;
    buf_write[*bufSize+offset] = FLAG;
    offset++;
    *bufSize+=offset;
    return 0;
}

int changeFrame(unsigned char* frame, int frameSize, int probability, int *index){
    
    *index = rand() % (frameSize);
    int originalByte = frame[*index];
    int change = rand() % (101);
    if(change < probability){
        frame[*index] ^= 0xFF;
        printf("Caused an artificial error at packet[%d] from %02X to %02X\n", *index, originalByte, frame[*index]);
        return originalByte;
    }
    return -1;
}

int changeFrameBack(unsigned char* frame, int index, int originalByte){
    frame[index] = originalByte;
    printf("The error was changed back at packet[%d] to %02X\n", index, frame[index]);
    return 0;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters){
    gettimeofday(&begin, NULL);
    printf("llopen start\n");

    statistics.nOfTimeouts=0;

    statistics.nOfBytesllopenSent=0;
    statistics.nOfPacketsllopenSent=0;
    statistics.nOfBytesllopenReceived=0;
    statistics.nOfPacketsllopenReceived=0;

    statistics.nOfBytesllwriteSent=0;
    statistics.nOfPacketsllwriteSent=0;
    statistics.nOfBytesllwriteReceived=0;
    statistics.nOfPacketsllwriteReceived=0;
    statistics.nOfCharStuffed=0;

    statistics.nOfBytesllreadReceived=0;
    statistics.nOfPacketsllreadReceived=0;
    statistics.nOfBytesllreadReceived=0;
    statistics.nOfPacketsllreadReceived=0;
    statistics.nOfREJSent=0;
    statistics.nOfCharDestuffed=0;

    statistics.nOfBytesllcloseSent=0;
    statistics.nOfPacketsllcloseSent=0;
    statistics.nOfBytesllcloseReceived=0;
    statistics.nOfPacketsllcloseReceived=0;

    connectionParam = connectionParameters;

    fd = connect(connectionParameters.serialPort);
    if(fd < 0) return -1;
    State state = FIRSTFLAG;
    unsigned char buf_read[2] = {0};
    switch(connectionParameters.role){
        case LlTx: {
            nRetransmissions = connectionParameters.nRetransmissions;
            (void)signal(SIGALRM,alarmHandler);
            while(nRetransmissions > 0){
                if (!alarmEnabled){ // enables the timer with timeout seconds 
                    alarm(connectionParameters.timeout);
                    alarmEnabled = TRUE;
                    int bytes = writeSupervisionFrame(0x03, SET);
                    printf("%d bytes written\n", bytes);
                    statistics.nOfBytesllopenSent+=bytes;
                    statistics.nOfPacketsllopenSent++;
                }
                STOP=FALSE;
                while(!STOP) {
                    int bytesRead = read(fd, buf_read, 1);
                    if(bytesRead!=0){
                        statistics.nOfBytesllopenReceived++;
                        if (state != FINALFLAG && buf_read[0] == FLAG) {
                            state = A;
                        }
                        else if (state == A && buf_read[0] == 0x03) {
                            state = C;
                        }
                        else if (state == C && buf_read[0] == UA) {
                            state = BCC1;
                        }
                        else if (state == BCC1 && buf_read[0] == (0x03 ^ UA)) {
                            state = FINALFLAG;
                        }
                        else if (state == FINALFLAG && buf_read[0] == FLAG) {
                            STOP = TRUE;
                            nRetransmissions=-1;
                            statistics.nOfPacketsllopenReceived++;
                        }
                        else state = FIRSTFLAG;
                    }
                }
            }
            if(nRetransmissions==0){
                printf("Ending execution due to timeout\n");
                exit(-1);
            }
            nRetransmissions = connectionParameters.nRetransmissions;
            alarmEnabled=FALSE;
            alarm(0); 
            break;
        }

        case LlRx: {
            while(!STOP){
                int bytesRead = read(fd, buf_read, 1);
                if(bytesRead!=0){
                    statistics.nOfBytesllopenReceived++;
                    if(state != FINALFLAG && buf_read[0]==FLAG){
                        state = A;
                    }
                    else if(state == A && buf_read[0]==0x03){
                        state = C;
                    }
                    else if(state == C && buf_read[0]==SET){
                        state = BCC1;
                    }
                    else if(state == BCC1 && buf_read[0]==(0x03 ^ SET)){
                        state = FINALFLAG;
                    }
                    else if(state == FINALFLAG && buf_read[0] == FLAG){
                        STOP=TRUE;
                        statistics.nOfPacketsllopenReceived++;
                    }
                    else state = FIRSTFLAG;
                }
            }
            int bytes = writeSupervisionFrame(0x03, UA);
            statistics.nOfBytesllopenSent+=bytes;
            statistics.nOfPacketsllopenSent++;
            printf("%d bytes written\n", bytes);
            break;
        }
    }
    STOP = FALSE;
    printf("LLOpen success\n");
    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize){
    printf("LLWrite start\n");
    
    State state = FIRSTFLAG;
    unsigned char* buf_write = (unsigned char*) malloc((bufSize*2)+8);;
    stuffBytes(buf_write,&bufSize,buf);

    unsigned char cField = 0x00;

    unsigned char buf_read[2] = {0};
    
    while (nRetransmissions > 0)
    { 
        if (alarmEnabled == FALSE)
        {   
            int index = 0;
            int byteChanged = changeFrame(buf_write, bufSize, PROBABILITY, &index);
            int bytes = write(fd, buf_write, bufSize);
            if(byteChanged != -1) changeFrameBack(buf_write, index, byteChanged);
            printf("%d bytes written\n", bytes);
            sleep(T_PROP);
            statistics.nOfBytesllwriteSent+=bufSize;
            statistics.nOfPacketsllwriteSent++;
            alarm(connectionParam.timeout); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
            
        }
        state = FIRSTFLAG;
        
        STOP=FALSE;
        while(!STOP) {
            int bytesRead = read(fd, buf_read, 1);
            if (bytesRead!=0){
                statistics.nOfBytesllwriteReceived++;
                if (state != FINALFLAG && buf_read[0] == FLAG) {
                    state = A;
                }
                else if (state == A && buf_read[0] == 0x03) {
                    state = C;
                }
                else if (state == C) {
                    state = BCC1;
                    cField = buf_read[0];
                } 
                else if (state == BCC1 && buf_read[0] == (0x03 ^ cField)) {
                    state = FINALFLAG;
                } 
                else if (state == FINALFLAG && buf_read[0] == FLAG) {
                    if (cField == UA) {
                        state = SUCCESS;
                    }
                    else if (cField == RR0) {
                        if(frame==FRAME0){
                            state=FAILURE;
                        }
                        else{
                            frame = FRAME0;
                            state = SUCCESS;
                            printf("Got success in RR0\n");
                        }
                    }
                    else if (cField == RR1) {
                        if(frame==FRAME1){
                            state=FAILURE;
                        }
                        else{
                            frame = FRAME1;
                            state = SUCCESS;
                            printf("Got success in RR1\n");
                        }
                    }
                    else if (cField == REJ0 || cField == REJ1 || cField == DISC) {
                        state=FAILURE;
                    }
                }
                else state = FIRSTFLAG;
            }
            if (state==SUCCESS){
                STOP = TRUE;
                alarmEnabled=FALSE;
                nRetransmissions=-1;
                statistics.nOfPacketsllwriteReceived++;
            }
            else if (state==FAILURE){
                STOP = TRUE;
                printf("Received REJ or failure in the response frame\n");
                alarmEnabled=FALSE;
                state=FIRSTFLAG;
                nRetransmissions = connectionParam.nRetransmissions;
            }
        }
    }
    // End execution at the end of the number of tries
    if(nRetransmissions==0){
        printf("Ending execution due to timeout\n");
        exit(-1);
    }
    nRetransmissions = connectionParam.nRetransmissions;
    alarmEnabled=FALSE;
    alarm(0);
    STOP=FALSE;
    printf("\n\nLLWrite successful\n\n");
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *output)
{
    printf("LLRead start\n");
    unsigned char buf_read[2] = {0};
    int responseFrame=0;
    int currentIndex=0;
    int bufSize=0;
    unsigned char bcc2Field=0x00;
    unsigned char cField=0x00;
    State state = FIRSTFLAG;
    while(!STOP){
        int bytesRead = read(fd, buf_read, 1);
        if(bytesRead!=0){
            statistics.nOfBytesllreadReceived++;
            if(state!=DATA && state!=FINALFLAG && buf_read[0] == FLAG){
                state = A;
            }
            else if(state==FIRSTFLAG){
                state = FIRSTFLAG;
            }
            else if(state == A && buf_read[0]==0x03){
                state = C;
            }
            else if(state == C){
                state=BCC1;
                cField=buf_read[0];
            }
            else if(state == BCC1 && buf_read[0]==(0x03 ^ cField)){
                if(cField == DISC){
                    state = FINALFLAG;
                }
                else if (cField==SET)
                {
                    printf("Transmitter stuck in LLOpen\n");
                    state=FINALFLAG;
                }
                else{
                    state = DATA;
                }
            }
            else if(state == DATA){
                if(currentIndex!=0 && buf_read[0]==0x5E && output[currentIndex-1]==0x7D){
                    output[currentIndex-1]=0x7E;
                    bcc2Field ^= output[currentIndex-2];
                    statistics.nOfCharDestuffed++;
                }
                else if(currentIndex!=0 && buf_read[0]==0x5D && output[currentIndex-1]==0x7D){
                    output[currentIndex-1]=0x7D;
                    bcc2Field ^= output[currentIndex-2];
                    statistics.nOfCharDestuffed++;
                }
                else{
                    output[currentIndex]=buf_read[0];
                    if(currentIndex>1){
                        if(output[currentIndex-1]!=0x7E && output[currentIndex-1]!=0x7D){
                            bcc2Field ^= output[currentIndex-2];
                        }
                    }
                    if(buf_read[0] != FLAG) {
                        currentIndex++;
                    }
                }
                if(buf_read[0] == FLAG) {
                    if(bcc2Field==output[currentIndex-1]){
                        state = SUCCESS; 
                        output[currentIndex-1]=0;
                        currentIndex--;
                    }
                    else{
                        state=FAILURE;
                    }
                }
            }
            else if(state == FINALFLAG && buf_read[0] == FLAG) {
                if(cField==SET){
                    int bytes = writeSupervisionFrame(0x03, UA);
                    statistics.nOfBytesllopenSent+=bytes;
                    statistics.nOfPacketsllopenSent++;
                    printf("UA sent by llread\n");
                    state=FIRSTFLAG;
                }
                else if(cField == frame){
                    state = SUCCESS;
                }
                else if (cField == DISC) {
                    state = DISCONNECTING;
                }
                else{
                    state = FAILURE;
                }
            }
            else state = FAILURE;
        }
        if(state==SUCCESS){
            bufSize=currentIndex;
            if(frame==FRAME0){
                frame=FRAME1;
                responseFrame = RR1;
            }
            else{
                frame=FRAME0;
                responseFrame = RR0;
            }
            int bytes =writeSupervisionFrame(0x03, responseFrame);
            printf("Success frame sent with 0x%02X\n",responseFrame);
            statistics.nOfPacketsllreadReceived++;
            statistics.nOfBytesllreadSent+=bytes;
            statistics.nOfPacketsllreadSent++;
            state=FIRSTFLAG;
            STOP=TRUE;
        }
        else if (state==FAILURE) {
            if(cField==FRAME0){
                responseFrame = REJ0;
            }
            else{
                responseFrame = REJ1;
            }
            writeSupervisionFrame(0x03, responseFrame);
            printf("Failure frame sent with 0x%02X\n",responseFrame);
            statistics.nOfREJSent++;
            bcc2Field=0x00;
            currentIndex=0;
            state=FIRSTFLAG;
        }
        else if (state==DISCONNECTING){
            STOP=TRUE;
            bufSize=-2;
        }
    }
    STOP=FALSE;
    printf("LLRead successful\n\n");
    return bufSize;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics){
    printf("LLClose start\n");

    unsigned char buf_read[2] = {0};

    unsigned char aField=0x00;
    unsigned char cField=0x00;

    State state = FIRSTFLAG;
    
    (void)signal(SIGALRM,alarmHandler);
    switch(connectionParam.role) {
        case LlTx:{
            while(nRetransmissions > 0){
                if (!alarmEnabled){ // enables the timer with timeout seconds 
                    alarm(connectionParam.timeout);
                    alarmEnabled = TRUE;
                    int bytes = writeSupervisionFrame(0x03,DISC);
                    printf("%d bytes written\n", bytes);
                    statistics.nOfBytesllcloseSent+=bytes;
                    statistics.nOfPacketsllcloseSent++;
                }
                STOP=FALSE;
                while(!STOP) {
                    int bytesRead = read(fd, buf_read, 1);
                    if(bytesRead!=0){
                        statistics.nOfBytesllcloseReceived++;
                        if (state != FINALFLAG && buf_read[0] == FLAG) {
                            state = A;
                        }
                        else if (state == A && buf_read[0] == 0x01) {
                            state = C;
                        }
                        else if (state == C && buf_read[0] == DISC) {
                            state = BCC1;
                        }
                        else if (state == BCC1 && buf_read[0] == (0x01 ^ DISC)) {
                            state = FINALFLAG;
                        }
                        else if (state == FINALFLAG && buf_read[0] == FLAG) {
                            STOP = TRUE;
                            statistics.nOfPacketsllcloseReceived++;
                            nRetransmissions = -1;
                            alarmEnabled=FALSE;
                            printf("Success\n");
                            state=SUCCESS;
                        }
                        else if (state==FAILURE){
                            STOP=TRUE;
                            alarmEnabled=FALSE;
                            state=FIRSTFLAG;
                        }
                        else state = FAILURE;
                    }
                }
            }
            int bytes = writeSupervisionFrame(0x01,UA);
            statistics.nOfBytesllcloseSent+=bytes;
            statistics.nOfPacketsllcloseSent++;
            if(nRetransmissions==0){
                printf("Ending execution due to timeout\n");
                exit(-1);
            }
            else{
                printf("\n\nLLClose successful\n\n");
            }
            nRetransmissions = connectionParam.nRetransmissions;
            if(showStatistics){
                gettimeofday(&end, NULL);
                double time_spent = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) / 1000000.0;
                //Print all statistics from the statistic struct formated in a table form
                printf("\n|----------------------------------------------|\n");
                printf("|------------Transmitter statistics------------|\n");
                printf("|----------------------------------------------|\n");
                printf("| Number of timeouts                 | %-7d |\n", statistics.nOfTimeouts);
                printf("|----------------------------------------------|\n");
                printf("| Number of bytes llopen sent        | %-7d |\n", statistics.nOfBytesllopenSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of packets llopen sent      | %-7d |\n", statistics.nOfPacketsllopenSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of bytes llopen received    | %-7d |\n", statistics.nOfBytesllopenReceived);
                printf("|----------------------------------------------|\n");
                printf("| Number of packets llopen received  | %-7d |\n", statistics.nOfPacketsllopenReceived);
                printf("|----------------------------------------------|\n");
                printf("| Number of bytes llwrite sent       | %-7d |\n", statistics.nOfBytesllwriteSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of packets llwrite sent     | %-7d |\n", statistics.nOfPacketsllwriteSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of bytes llwrite received   | %-7d |\n", statistics.nOfBytesllwriteReceived);
                printf("|----------------------------------------------|\n");
                printf("| Number of packets llwrite received | %-7d |\n", statistics.nOfPacketsllwriteReceived);
                printf("|----------------------------------------------|\n");
                printf("| Number of bytes llclose received   | %-7d |\n", statistics.nOfBytesllcloseReceived);
                printf("|----------------------------------------------|\n");
                printf("| Number of packets llclose received | %-7d |\n", statistics.nOfPacketsllcloseReceived);
                printf("|----------------------------------------------|\n");
                printf("| Number of bytes llclose sent       | %-7d |\n", statistics.nOfBytesllcloseSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of packets llclose sent     | %-7d |\n", statistics.nOfPacketsllcloseSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of stuffed characters       | %-7d |\n", statistics.nOfCharStuffed);
                printf("|----------------------------------------------|\n");
                printf("| Execution time in seconds          | %.4lf |\n",time_spent);
                printf("|----------------------------------------------|\n");
            }
            break;
        }
        case LlRx:{
            int bytes = writeSupervisionFrame(0x01,DISC);
            statistics.nOfBytesllcloseSent+=bytes;
            statistics.nOfPacketsllcloseSent++;
            nRetransmissions = 1;
            if (!alarmEnabled){  
                alarm(connectionParam.timeout);
                alarmEnabled = TRUE;
                statistics.nOfBytesllcloseSent+=bytes;
                statistics.nOfPacketsllcloseSent++;
            }
            while(!STOP){
                int bytesRead = read(fd, buf_read, 1);
                if(bytesRead!=0){
                    statistics.nOfBytesllcloseReceived++;
                    if(state != FINALFLAG && buf_read[0] == FLAG){
                        state = A;
                    }
                    else if(state == A && (buf_read[0]==0x03 || buf_read[0]==0x01)){
                        state = C;
                        aField=buf_read[0];
                    }
                    else if(state == C && ((aField==0x03 && buf_read[0]==DISC) || (aField==0x01 && buf_read[0]==UA))){
                        state = BCC1;
                        cField=buf_read[0];
                    }
                    else if(state == BCC1 && buf_read[0]==(aField ^ cField)){
                        state = FINALFLAG;
                    }
                    else if(state == FINALFLAG && buf_read[0] == FLAG){
                        if(cField==UA){
                            STOP=TRUE;
                            printf("Success\n");
                            statistics.nOfPacketsllcloseReceived++;
                        }
                        else{
                            int bytes = writeSupervisionFrame(0x01,DISC);
                            statistics.nOfBytesllcloseSent+=bytes;
                            statistics.nOfPacketsllcloseSent++;
                            state=FIRSTFLAG;
                        }
                    }
                    else state = FIRSTFLAG;
                }
            }
            if(nRetransmissions==0){
                printf("LLClose finished due to timeout\n");
            }
            else{
                printf("\n\nLLClose success\n\n");
            }
            if(showStatistics){
                gettimeofday(&end, NULL);
                double time_spent = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) / 1000000.0;
                double r = (v*8)/time_spent;
                double practical_efficiency = (r/connectionParam.baudRate)*100; 
                double FER = PROBABILITY;
                double tf = (double)((MAX_PAYLOAD_SIZE+6)*8)/connectionParam.baudRate;
                double pe = (double)FER/100;
                double theoretical_efficiency = (double)(1-pe)/(1+2*(T_PROP/tf));
                //Print all statistics from the statistic struct formated in a table form
                printf("\n|----------------------------------------------|\n");
                printf("|------------Receiver statistics---------------|\n");
                printf("|----------------------------------------------|\n");
                printf("| Number of bytes llopen sent        | %-7d |\n", statistics.nOfBytesllopenSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of packets llopen sent      | %-7d |\n", statistics.nOfPacketsllopenSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of bytes llopen received    | %-7d |\n", statistics.nOfBytesllopenReceived);
                printf("|----------------------------------------------|\n");
                printf("| Number of packets llopen received  | %-7d |\n", statistics.nOfPacketsllopenReceived);
                printf("|----------------------------------------------|\n");
                printf("| Number of bytes llread sent        | %-7d |\n", statistics.nOfBytesllreadSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of packets llread sent      | %-7d |\n", statistics.nOfPacketsllreadSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of bytes llread received    | %-7d |\n", statistics.nOfBytesllreadReceived);
                printf("|----------------------------------------------|\n");
                printf("| Number of packets llread received  | %-7d |\n", statistics.nOfPacketsllreadReceived);
                printf("|----------------------------------------------|\n");
                printf("| Number of REJ's sent               | %-7d |\n", statistics.nOfREJSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of bytes llclose received   | %-7d |\n", statistics.nOfBytesllcloseReceived);
                printf("|----------------------------------------------|\n");
                printf("| Number of packets llclose received | %-7d |\n", statistics.nOfPacketsllcloseReceived);
                printf("|----------------------------------------------|\n");
                printf("| Number of bytes llclose sent       | %-7d |\n", statistics.nOfBytesllcloseSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of packets llclose sent     | %-7d |\n", statistics.nOfPacketsllcloseSent);
                printf("|----------------------------------------------|\n");
                printf("| Number of destuffed characters     | %-7d |\n", statistics.nOfCharDestuffed);
                printf("|----------------------------------------------|\n");
                printf("| R                                  | %.2lf |\n", r);
                printf("|----------------------------------------------|\n");
                printf("| FER                                | %.3lf%% |\n", FER);
                printf("|----------------------------------------------|\n");
                printf("| Practical efficiency               | %.3lf%% |\n", practical_efficiency);
                printf("|----------------------------------------------|\n");
                printf("| Theoretical efficiency             | %.3lf%% |\n", theoretical_efficiency*100);
                printf("|----------------------------------------------|\n");
                printf("| TF                                 | %.5lf |\n",tf);
                printf("|----------------------------------------------|\n");
                printf("| Execution time in seconds          | %.4lf |\n",time_spent);
                printf("|----------------------------------------------|\n");

            }
            break;
        }
    }

    sleep(1);

    if (tcsetattr(filePort, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }
    close(fd); //closes the serial port
    return 0;
}
