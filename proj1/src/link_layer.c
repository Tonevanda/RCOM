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
    if(fd < 0) return -1;
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
    unsigned char input[BUF_SIZE];
    input[0] = 0x6F;
    input[1] = 0x69;
    input[2] = 0x20;
    input[3] = 0x6A;
    input[4] = 0x6F;
    input[5] = 0x6D;
    input[6] = 0x69;
    input[7] = 0x69;
    input[8] = 0x69;
    input[9] = '\n';
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
            alarm(connectionParam.timeout); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }

        state = FIRSTFLAG;
        unsigned char cField = 0x00;
        STOP=FALSE;
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
                    nRetransmissions = connectionParam.nRetransmissions;
                }
                cField = buf_read[0];
            } else if (state == BCC1 && buf_read[0] == (0x01 ^ cField)) {
                state = FINALFLAG;
            } else if (state == FINALFLAG && buf_read[0] == FLAG) {
                STOP = TRUE;
                alarmEnabled=FALSE;
                if(input[firstDataByte+3]!='\n'){
                    nRetransmissions = connectionParam.nRetransmissions;
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
    nRetransmissions = connectionParam.nRetransmissions;
    STOP=FALSE;
    printf("\n\nllwrite success\n\n");
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    printf("llread start\n\n");
    int responseFrame=0;
    int firstDataByte=0;
    unsigned char frame=0x00;
    unsigned char buf_read[2] = {0};
    unsigned char output[BUF_SIZE]={0};
    State state = FIRSTFLAG;
    while (state != DISCONNECTING){
        state = FIRSTFLAG;
        while(!STOP){
            int bytes = read(fd, buf_read, 1);
            printf("test char= 0x%02X | state= %d\n", buf_read[0],state);
            if(state != FIRSTFLAG && state != FAILURE){
                if(bytes != 0){
                    printf("char= 0x%02X | ", buf_read[0]);
                }
                printf("state= %d\n",state);
            }
            if(state!=FINALFLAG && buf_read[0] == FLAG){
                printf("char= 0x%02X | state= %d\n", buf_read[0],state);
                state = A;
            }
            else if(state==FIRSTFLAG){
                state = FIRSTFLAG;
            }
            else if(state == A && buf_read[0]==0x03){
                state = C;
            }
            else if(state == C){
                //wip frame
                if(buf_read[0]==FRAME0){
                    if(responseFrame == RR1){
                        firstDataByte-=3;
                    }
                    responseFrame = RR1;
                }
                else if(buf_read[0]==FRAME1){
                    if(responseFrame == RR0){
                        firstDataByte-=3;
                    }
                    responseFrame = RR0;
                }
                state=BCC1;
                frame=buf_read[0];
            }
            else if(state == BCC1 && buf_read[0]==(0x03 ^ frame)){
                if(frame == DISC){
                    state = FINALFLAG;
                }
                else{
                    state = DATA;
                }
            }
                //wip DATA state
            else if(state == DATA){
                printf("\ndata\n");
                output[firstDataByte]=buf_read[0];
                firstDataByte++;
                if(firstDataByte % 3 == 0) {
                    state = BCC2;
                }
            }
            else if(state == BCC2){
                if(buf_read[0] == (output[firstDataByte-3] ^ output[firstDataByte-2] ^ output[firstDataByte-1]))
                    state = FINALFLAG;
                else{
                    state = FAILURE;
                    firstDataByte-=3;
                }
            }
            else if(state == FINALFLAG) {
                if (buf_read[0] == FLAG) {
                    STOP = TRUE;
                    printf("success");
                    state = SUCCESS;
                } else {
                    state = FAILURE;
                    firstDataByte -= 3;
                }
                if (frame == DISC) {
                    printf("\n\n\nsuccess in disconecting\n");
                    state = DISCONNECTING;
                }
            }
            else if (state==FAILURE){
                STOP=TRUE;
            }
            else state = FAILURE;
        }

        if(state==SUCCESS){
            printf("sucsess frame sent\n");

            writeSupervisionFrame(0x01, responseFrame);

            STOP=FALSE;
        }
        else if (state==FAILURE) {
            printf("failure frame sent\n");
            if(responseFrame==RR0){
                responseFrame=REJ1;
            }
            else{
                responseFrame=REJ0;
            }
            writeSupervisionFrame(0x01, responseFrame);
            STOP=FALSE;
        }
    }
    STOP=FALSE;
    printf("\n\nllread success\n\n");
    output[firstDataByte]='\n';
    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics){
    printf("llclose start\n\n");

    unsigned char buf_read[2] = {0};
    unsigned char buf_write[BUF_SIZE] = {0};

    unsigned char aField=0x00;
    unsigned char cField=0x00;

    State state = FIRSTFLAG;

    nRetransmissions = connectionParam.nRetransmissions;

    int bytesWritten = writeSupervisionFrame(0x01,DISC);
    printf("%d bytes written | ", bytesWritten);

    switch(connectionParam.role) {
        case LlTx:{
            while(nRetransmissions > 0){
                int bytesRead = read(fd, buf_read, 1);
                if(state != FIRSTFLAG){
                    if(bytesRead != 0){
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
                write(fd, buf_write, BUF_SIZE);
                printf("resent disc\n");
                if (alarmEnabled == FALSE)
                {
                    alarm(connectionParam.timeout);
                    alarmEnabled = TRUE;
                }
            }
            writeSupervisionFrame(0x01,UA);
            nRetransmissions = connectionParam.nRetransmissions;
            break;
        }
        case LlRx:{
            while(!STOP){
                int bytesRead = read(fd, buf_read, 1);
                if(state != FIRSTFLAG){
                    if(bytesRead != 0){
                        printf("char= 0x%02X | ", buf_read[0]);
                    }
                    printf("state= %d\n",state);
                }

                if(state != FINALFLAG && buf_read[0] == FLAG){
                    printf("char= 0x%02X | state= %d\n", buf_read[0],state);
                    state = A;
                }
                else if(state == A && (buf_read[0]==0x03 || buf_read[0]==0x01)){
                    state = C;
                    aField=buf_read[0];
                }
                else if(state == C){
                    if((aField==0x03 && buf_read[0]==DISC) || (aField==0x01 && buf_read[0]==UA)){
                        cField=buf_read[0];
                        state = BCC1;
                    }
                    else state = FIRSTFLAG;
                }
                else if(state == BCC1 && buf_read[0]==(aField ^ cField)){
                    state = FINALFLAG;
                }
                else if(state == FINALFLAG && buf_read[0] == FLAG){
                    if(cField==UA){
                        STOP=TRUE;
                        printf("success");
                    }
                    else{
                        write(fd, buf_write, BUF_SIZE);
                        state=FIRSTFLAG;
                    }
                }
                else state = FIRSTFLAG;
            }
            break;
        }
    }

    close(fd); //closes the serial port
    printf("\n\nllclose success\n\n");
    return 0;
}


int connect(const char *serialPort){
    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int file = open(serialPort, O_RDWR | O_NOCTTY);
    printf("connected\n");
    if (file < 0)
    {
        perror(serialPort);
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
    int bytesWritten = 0;
    if((bytesWritten = write(fd, supervisionFrame, 6)) < 0) return -1;
    sleep(1);
    return bytesWritten;
}