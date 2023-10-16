// Link layer protocol implementation

#include "link_layer.h"


volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;
int alarmTimeout = 0;
int nRetransmissions = 0;
int fd;
LinkLayer connectionParam;


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters){
    connectionParam = connectionParameters;
    printf("llopen start\n\n");
    fd = connect(connectionParameters.serialPort);
    if(fd != 0) return -1;
    State state = FIRSTFLAG;
    unsigned char buf_read[2] = {0};
    nRetransmissions = connectionParameters.nRetransmissions;

    switch(connectionParameters.role){

        case LlTx: {

            while(nRetransmissions > 0){

                int bytes = writeSupervisionFrame(0x03, SET);
                printf("%d bytes written\n", bytes);
                if (!alarmEnabled){ // enables the timer with timeout seconds 
                    alarm(connectionParameters.timeout);
                    alarmEnabled = TRUE;
                }

                while(!STOP) {
                    int bytesRead = read(fd, buf_read, 1);
                    if(state != FIRSTFLAG){
                        if(bytesRead != 0){
                            printf("char= 0x%02X | ", buf_read[0]);
                        }
                        printf("state= %d\n",state);
                    }

                    if (state != FINALFLAG && buf_read[0] == FLAG) {
                        printf("char= 0x%02X | state= %d\n", buf_read[0],state);
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
                        alarmEnabled=FALSE;
                    }
                    else state = FIRSTFLAG;
                }
            }
            break;
        }

        case LlRx: {
                while(!STOP){
                    int bytes = read(fd, buf_read, 1);
                    if(state != FIRSTFLAG){
                        if(bytes != 0){
                            printf("char= 0x%02X | ", buf_read[0]);
                        }
                        printf("state= %d\n",state);
                    }
                    
                    if(state != FINALFLAG && buf_read[0]==FLAG){
                        printf("char= 0x%02X | state= %d\n", buf_read[0],state);
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
                    }
                    else state = FIRSTFLAG;
                }

                int bytes = writeSupervisionFrame(0x03, UA);

                printf("%d bytes written\n", bytes);

                sleep(1);
            break;
        }

    }

    nRetransmissions = connectionParameters.nRetransmissions;
    STOP = FALSE;
    printf("\n\nllopen success\n\n");
    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    printf("llwrite start\n\n");
    int firstDataByte=0;
    unsigned char buf_write[BUF_SIZE] = {0};
    unsigned char buf_read[2] = {0};
    State state = FIRSTFLAG;
    //Set up header
    buf_write[0] = FLAG;
    buf_write[1] = 0x03;

    buf_write[8] = FLAG;
    buf_write[9] = '\n';

    unsigned char frame=0x00;
    while (nRetransmissions > 0)
    {
        buf_write[2] = frame;
        buf_write[3] = buf_write[1] ^ buf_write[2];
        buf_write[4] = input[firstDataByte];
        buf_write[5] = input[firstDataByte+1];
        buf_write[6] = input[firstDataByte+2];
        buf_write[7] = buf_write[4] ^ buf_write[5] ^ buf_write[6];
        int bytes = write(fd, buf_write, BUF_SIZE);
        printf("%d bytes written\n", bytes);
        // Wait until all bytes have been written to the serial port
        sleep(1);

        if (alarmEnabled == FALSE)
        {
            alarm(3); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }

        state = FIRSTFLAG;
        unsigned char c = 0x00;
        STOP=FALSE;
        while(!STOP) {
            int bytes = read(fd, buf_read, 1);
            if(state != FIRSTFLAG){
                if(bytes != 0){
                    printf("char= 0x%02X | ", buf_read[0]);
                }
                printf("state= %d\n",state);
            }

            if (state != FINALFLAG && buf_read[0] == FLAG) {
                printf("char= 0x%02X | state= %d\n", buf_read[0],state);
                state = A;
            }
            else if (state == A && buf_read[0] == 0x01) {
                state = C;
            }
            else if (state == C) {
                if (buf_read[0] == UA) {
                    state = BCC1;
                }
                else if (buf_read[0] == RR0) {
                    if(frame==FRAME0){
                        firstDataByte-=3;
                    }
                    frame = FRAME0;
                    state = BCC1;
                }
                else if (buf_read[0] == RR1) {
                    if(frame==FRAME1){
                        firstDataByte-=3;
                    }
                    frame = FRAME1;
                    state = BCC1;
                }
                else if (buf_read[0] == REJ0 || buf_read[0] == REJ1 || buf_read[0] == DISC) {
                    firstDataByte-=3;
                    if(frame==FRAME1){
                        frame=FRAME0;
                    }
                    else{
                        frame=FRAME1;
                    }
                    STOP=TRUE;
                    alarmEnabled=FALSE;
                    nRetransmissions = connectionParameters.nRetransmissions;
                }
                c = buf_read[0];
            } else if (state == BCC1 && buf_read[0] == (0x01 ^ c)) {
                state = FINALFLAG;
            } else if (state == FINALFLAG && buf_read[0] == FLAG) {
                STOP = TRUE;
                alarmEnabled=FALSE;
                if(input[firstDataByte+3]!='\n'){
                    nRetransmissions = connectionParameters.nRetransmissions;
                    firstDataByte+=3;
                    printf("one transfer complete");
                }
                else{
                    nRetransmissions=-1;
                    printf("final transfer complete");
                }
            }
            else state = FIRSTFLAG;
        }
    }
    nRetransmissions = connectionParameters.nRetransmissions;
    STOP=FALSE;
    printf("\n\nllwrite success\n\n");
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    printf("llclose start\n\n");

    unsigned char buf_read[2] = {0};
    unsigned char buf_write[BUF_SIZE] = {0};

    unsigned char aField=0x00;
    unsigned char cField=0x00;

    State state = FIRSTFLAG;

    nRetransmissions = connectionParameters.nRetransmissions;

    int bytes = writeSupervisionFrame(0x01,DISC); //talvez de merda porque nao meto \n no final?????
    printf("%d bytes written | ", bytes);

    switch(connectionParam.role) {
        case LlTx:{
            while(nRetransmissions > 0){
                int bytes = read(fd, buf_read, 1);
                if(state != FIRSTFLAG){
                    if(bytes != 0){
                        printf("char= 0x%02X\n", buf_read[0]);
                    }
                    printf("state= %d\n",state);
                }

                if (state != FINALFLAG && buf_read[0] == FLAG) {
                    printf("char= 0x%02X | state= %d\n", buf_read[0],state);
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
                    nRetransmissions = -1;
                    alarmEnabled=FALSE;
                    printf("success");
                    state=SUCCESS;
                }
                else state = FIRSTFLAG;
            }
            if (state!=SUCCESS){
                bytes = write(fd, buf_write, BUF_SIZE);
                printf("resent disc\n");
                if (alarmEnabled == FALSE)
                {
                    alarm(connectionParam.timeout);
                    alarmEnabled = TRUE;
                }
            }
            bytes = writeSupervisionFrame(0x01,UA);
            nRetransmissions = connectionParam.nRetransmissions;
            STOP = FALSE;
            break;
        }
        case LlRx:{

            break;
        }
    }

    close(fd);
    printf("\n\nllclose success\n\n");
    return 0;
}


int connect(const char *serialPort){
    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int file = open(serialPortName, O_RDWR | O_NOCTTY);
    if (file < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(file, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
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
    tcflush(file, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(file, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    return file;
}

void alarmHandler(int signal){
    alarmEnabled = FALSE;
    alarmCount++;
    STOP=TRUE;
    printf("Alarm #%d\n", alarmCount);
}

int writeSupervisionFrame(unsigned char A, unsigned char C){
    unsigned char supervisionFrame[6] = {FLAG, A, C, A ^ C, FLAG, '\n'};
    if(write(fd, supervisionFrame, 6) != 0) return -1;
    return 0;
}