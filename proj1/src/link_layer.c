// Link layer protocol implementation

#include "link_layer.h"


volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmTimeout = 0;
int nRetransmissions = 0;
unsigned char frame=0x00;
int fd;
LinkLayer connectionParam;
Statistics statistics;
clock_t begin;


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters){

    begin = clock();

    printf("llopen start\n\n");
    statistics.nOfTimeouts=0;

    statistics.nOfBytesllopenSent=0;
    statistics.nOfPacketsllopenSent=0;
    
    statistics.nOfBytesllopenRecieved=0;
    statistics.nOfPacketsllopenRecieved=0;

    statistics.nOfBytesllwriteSent=0;
    statistics.nOfPacketsllwriteSent=0;
    statistics.nOfCharStuffed=0;

    statistics.nOfBytesllreadReceived=0;
    statistics.nOfPacketsllreadReceived=0;
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
    nRetransmissions = connectionParameters.nRetransmissions;

    switch(connectionParameters.role){

        case LlTx: {

            (void)signal(SIGALRM,alarmHandler);
            while(nRetransmissions > 0){

                int bytes = writeSupervisionFrame(0x03, SET);
                printf("%d bytes written\n", bytes);

                statistics.nOfBytesllopenSent+=bytes;
                statistics.nOfPacketsllopenSent++;
                if (!alarmEnabled){ // enables the timer with timeout seconds 
                    alarm(connectionParameters.timeout);
                    alarmEnabled = TRUE;
                }

                while(!STOP) {
                    int bytesRead = read(fd, buf_read, 1);
                    if(bytesRead!=0){
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
                            statistics.nOfBytesllopenRecieved+=5;
                            statistics.nOfPacketsllopenRecieved++;
                            nRetransmissions=-1;
                            alarmEnabled=FALSE;
                        }
                        else state = FIRSTFLAG;
                    }
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
                        statistics.nOfBytesllopenRecieved+=5;
                        statistics.nOfPacketsllopenRecieved++;
                    }
                    else state = FIRSTFLAG;
                }

                int bytes = writeSupervisionFrame(0x03, UA);
                statistics.nOfBytesllopenSent+=bytes;
                statistics.nOfPacketsllopenSent++;
                printf("%d bytes written\n", bytes);

            break;
        }

    }
    alarmEnabled=FALSE;
    alarm(0);
    nRetransmissions = connectionParameters.nRetransmissions;
    STOP = FALSE;
    printf("\n\nllopen success\n\n");
    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize){
    
    printf("llwrite start\n\n");
    unsigned char* buf_write = (unsigned char*) malloc((bufSize*2)+8);
    buf_write[0] = FLAG;
    buf_write[1] = 0x03;
    buf_write[2] = frame;
    buf_write[3] = buf_write[1] ^ buf_write[2];
    int offset=4;
    unsigned char tmp=0x00;
    for(int i=0;i<bufSize;i++){
        
        if(buf[i]==0x7E){
            buf_write[i+offset]=0x7D;
            offset++;
            buf_write[i+offset]=0x5E;
        }
        else if (buf[i]==0x7D)
        {
            buf_write[i+offset]=0x7D;
            offset++;
            buf_write[i+offset]=0x5D;
        }
        else{
            buf_write[i+offset]=buf[i];
        } 
        tmp^=buf[i];
    }
    unsigned char buf_read[2] = {0};
    State state = FIRSTFLAG;
    //Set up header
    printf("tmp= 0x%02X\n", tmp);
    if(tmp==0x7E){
        buf_write[bufSize+offset] = 0x7D;
        offset++;
        buf_write[bufSize+offset] = 0x5E;
    }
    else if (tmp==0x7D)
    {
        buf_write[bufSize+offset] = 0x7D;
        offset++;
        buf_write[bufSize+offset] = 0x5D;
    }
    else{
        buf_write[bufSize+offset] = tmp;
    }
    sleep(1);
    offset++;
    buf_write[bufSize+offset] = FLAG;
    offset++;
    buf_write[bufSize+offset] = '\n';
    while (nRetransmissions > 0)
    { 
        int bytes = write(fd, buf_write, bufSize+offset);
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
                        STOP=TRUE;
                        alarmEnabled=FALSE;
                    }
                    frame = FRAME0;
                    state = BCC1;
                }
                else if (buf_read[0] == RR1) {
                    if(frame==FRAME1){
                        STOP=TRUE;
                        alarmEnabled=FALSE;
                    }
                    frame = FRAME1;
                    state = BCC1;
                }
                else if (buf_read[0] == REJ0 || buf_read[0] == REJ1 || buf_read[0] == DISC) {
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
                nRetransmissions=-1;
                printf("final transfer complete");
            }
            else state = FIRSTFLAG;
        }
    }
    nRetransmissions = connectionParam.nRetransmissions;
    alarmEnabled=FALSE;
    alarm(0);
    STOP=FALSE;
    statistics.nOfPacketsllwriteSent++;
    printf("\n\nllwrite success\n\n");
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *output,int* bufSize)
{
    printf("llread start\n\n");
    int responseFrame=0;
    unsigned char buf_read[2] = {0};
    
    int firstDataByte=0;
    State state = FIRSTFLAG;
    unsigned char tmp=0x00;
    unsigned char tmpframe=0x00;
    firstDataByte=0;
    state = FIRSTFLAG;
    while(!STOP){
        int bytesRead = read(fd, buf_read, 1);
        if(state != FIRSTFLAG){
                if(bytesRead != 0){
                    printf("char= 0x%02X | ", buf_read[0]);
                    printf("state= %d\n",state);
                }
            }
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
            //wip frame
            if(buf_read[0]==frame){
                if(frame==FRAME0){
                    responseFrame = RR1;
                }
                else{
                    responseFrame = RR0;
                }
                state=BCC1;
                tmpframe=buf_read[0];
            }
            else if(buf_read[0]==FRAME0 || buf_read[0]==FRAME1){
                if(buf_read[0]==FRAME0){
                    responseFrame = REJ0;
                }
                else{
                    responseFrame = REJ1;
                }
                STOP=TRUE;
                state=FAILURE;
            }
            else{
                state=BCC1;
                tmpframe=buf_read[0];
            }
        }
        else if(state == BCC1 && buf_read[0]==(0x03 ^ tmpframe)){
            if(frame == DISC){
                state = FINALFLAG;
            }
            else{
                state = DATA;
            }
        }
        else if(state == DATA){
            if(firstDataByte!=0 && buf_read[0]==0x5E && output[firstDataByte-1]==0x7D){
                output[firstDataByte-1]=0x7E;
                tmp ^= output[firstDataByte-2];
            }
            else if(firstDataByte!=0 && buf_read[0]==0x5D && output[firstDataByte-1]==0x7D){
                output[firstDataByte-1]=0x7D;
                tmp ^= output[firstDataByte-2];
            }
            else{
                output[firstDataByte]=buf_read[0];
                if(firstDataByte>1){
                    if(output[firstDataByte-1]!=0x7E && output[firstDataByte-1]!=0x7D){
                        tmp ^= output[firstDataByte-2];
                    }
                }
                if(buf_read[0] != FLAG) {
                    firstDataByte++;
                }
            }
            if(buf_read[0] == FLAG) {
                if(tmp==output[firstDataByte-1]){
                    STOP = TRUE;
                    printf("success\n");
                    state = SUCCESS; 
                    output[firstDataByte-1]=0;
                    firstDataByte--;
                }
                else{
                    printf("failiure\n");
                    state=FAILURE;
                }
            }
        }
        else if(state == FINALFLAG) {
            if (buf_read[0] == FLAG) {
                STOP = TRUE;
                state = SUCCESS;
            } else {
                state = FAILURE;
                firstDataByte -= 3;
            }
            if (frame == DISC) {
                state = DISCONNECTING;
            }
        }
        else state = FAILURE;
    }

    if(state==SUCCESS){
        printf("sucsess frame sent\n");
        *bufSize=firstDataByte;
        if(frame==FRAME0){
            frame=FRAME1;
        }
        else{
            frame=FRAME0;
        }
        writeSupervisionFrame(0x01, responseFrame);
        statistics.nOfPacketsllreadReceived++;
        statistics.nOfBytesllreadReceived+=5;
        
        state=FIRSTFLAG;
        STOP=FALSE;
    }
    else if (state==FAILURE) {
        printf("failure frame sent\n");
        writeSupervisionFrame(0x01, responseFrame);
        state=FIRSTFLAG;
        STOP=FALSE;
    }
    else if (state==DISCONNECTING){
        STOP=FALSE;
        printf("\n\nallread success\n\n");
        *bufSize=-1;
        output[firstDataByte]='\n';
    }
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
    
    
    switch(connectionParam.role) {
        case LlTx:{
            (void)signal(SIGALRM,alarmHandler);
            writeSupervisionFrame(0x03,DISC);
            while(nRetransmissions > 0){
                int bytesRead = read(fd, buf_read, 1);
                if(bytesRead!=0){
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
            }
            if (state!=SUCCESS){
                writeSupervisionFrame(0x03,DISC);
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
            writeSupervisionFrame(0x01,DISC);
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

    clock_t end = clock();    

    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Execution time : %lf seconds",time_spent);
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
    nRetransmissions--;
    statistics.nOfTimeouts++;
    STOP=TRUE;
    printf("Alarm #%d\n", nRetransmissions);
}

int writeSupervisionFrame(unsigned char A, unsigned char C){
    unsigned char supervisionFrame[6] = {FLAG, A, C, A ^ C, FLAG, '\n'};
    int bytesWritten = 0;
    if((bytesWritten = write(fd, supervisionFrame, 6)) < 0) return -1;
    sleep(1);
    return bytesWritten;
}
