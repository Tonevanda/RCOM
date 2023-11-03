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
struct timeval begin, end;
int v;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters){
    gettimeofday(&begin, NULL);
    printf("llopen start\n\n");

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
                        printf("char= 0x%02X | state= %d\n", buf_read[0], state);
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
            nRetransmissions = connectionParameters.nRetransmissions;
            alarmEnabled=FALSE;
            alarm(0); 
            if(nRetransmissions==0){
                printf("MASSIVE failure");
                exit(-1);
            }
            break;
        }

        case LlRx: {
            while(!STOP){
                int bytesRead = read(fd, buf_read, 1);
                if(bytesRead!=0){
                    statistics.nOfBytesllopenReceived++;
                    printf("char= 0x%02X | state= %d\n", buf_read[0], state);

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
    printf("\n\nllopen success\n\n");
    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize){
    printf("llwrite start\n\n");
    
    State state = FIRSTFLAG;
    unsigned char* buf_write = (unsigned char*) malloc((bufSize*2)+8);;
    stuffBytes(buf_write,&bufSize,buf);

    unsigned char cField = 0x00;

    unsigned char buf_read[2] = {0};
    
    while (nRetransmissions > 0)
    { 
        if (alarmEnabled == FALSE)
        {
            int bytes = write(fd, buf_write, bufSize);
            printf("%d bytes written\n", bytes);
            sleep(1);
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
                printf("char= 0x%02X | state= %d\n", buf_read[0], state);
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
                    printf("Entered FINALFLAG state\n");
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
                            printf("Got success in RR0");
                        }
                    }
                    else if (cField == RR1) {
                        if(frame==FRAME1){
                            state=FAILURE;
                        }
                        else{
                            frame = FRAME1;
                            state = SUCCESS;
                            printf("Got success in RR1");
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
                printf("final transfer complete\n");
            }
            else if (state==FAILURE){
                STOP = TRUE;
                printf("rej recieved or failiure in the response frame\n");
                alarmEnabled=FALSE;
                state=FIRSTFLAG;
                nRetransmissions = connectionParam.nRetransmissions;
            }
        }
    }
    nRetransmissions = connectionParam.nRetransmissions;
    alarmEnabled=FALSE;
    alarm(0);
    STOP=FALSE;
    if(nRetransmissions==0){
        printf("MASSIVE failure");
        exit(-1);
    }
    printf("\n\nllwrite success\n\n");
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *output)
{
    printf("llread start\n\n");
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
            if(state != DATA){
                printf("char= 0x%02X | state= %d\n", buf_read[0], state);
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
                state=BCC1;
                cField=buf_read[0];
            }
            else if(state == BCC1 && buf_read[0]==(0x03 ^ cField)){
                if(cField == DISC){
                    state = FINALFLAG;
                }
                else if (cField==SET)
                {
                    printf("transmiter stuck in llopen :(\n");
                    state=FINALFLAG;
                }
                else{
                    state = DATA;
                }
            }
            else if(state == DATA){
                //remove if bellow to print all data bytes
                if(bytesRead == 0){
                    printf("char= 0x%02X | ", buf_read[0]);
                    printf("bcc2Field= 0x%02X ", bcc2Field);
                    printf("from= 0x%02X | ", output[currentIndex-2]);
                    printf("state= %d\n",state);
                }
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
                    printf("char = 0x%02X | bcc2Field = 0x%02X | output[firstDataByte-1] = 0x%02X | state = %d | number of bytes received = %-d", buf_read[0],bcc2Field,output[currentIndex-1],state,statistics.nOfBytesllreadReceived);
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
            printf("sucsess frame sent with 0x%02X\n",responseFrame);
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
            printf("failure frame sent with 0x%02X\n",responseFrame);
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
    printf("llread success\n\n");
    return bufSize;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics){
    printf("llclose start\n\n");

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
                        printf("char= 0x%02X | state= %d\n", buf_read[0], state);
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
                            printf("success");
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
            nRetransmissions = connectionParam.nRetransmissions;
            if(nRetransmissions==0){
                printf("MASSIVE failure");
                exit(-1);
            }
            else{
                printf("\n\nllclose success\n\n");
            }
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
                alarm(7);
                alarmEnabled = TRUE;
                statistics.nOfBytesllcloseSent+=bytes;
                statistics.nOfPacketsllcloseSent++;
            }
            while(!STOP){
                int bytesRead = read(fd, buf_read, 1);
                if(bytesRead!=0){
                    statistics.nOfBytesllcloseReceived++;
                    printf("char= 0x%02X | state= %d\n", buf_read[0], state);
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
                            printf("success");
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
                printf("llclose finished due to timeout\n");
            }
            else{
                printf("\n\nllclose success\n\n");
            }
            if(showStatistics){
                gettimeofday(&end, NULL);
                double time_spent = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) / 1000000.0;
                double r = (v*8)/time_spent;
                double practical_efficiency = (r/connectionParam.baudRate)*100; 
                double FER = ((double)statistics.nOfREJSent/(statistics.nOfPacketsllreadReceived+statistics.nOfREJSent))*100;
                double tf = (double)((MAX_PAYLOAD_SIZE+6)*8)/connectionParam.baudRate;
                double pe = (double)FER/100;
                double theoretical_efficiency = (double)(1-pe)/(2+1*(tf/T_PROP));
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
                printf("| FER                                | %.4lf%% |\n", FER);
                printf("|----------------------------------------------|\n");
                printf("| Practical efficiency               | %.3lf%% |\n", practical_efficiency);
                printf("|----------------------------------------------|\n");
                printf("| Theoretical efficiency             | %.4lf%% |\n", theoretical_efficiency*100);
                printf("|----------------------------------------------|\n");
                printf("| TF                                 | %.4lf |\n",tf);
                printf("|----------------------------------------------|\n");
                printf("| Execution time in seconds          | %.4lf |\n",time_spent);
                printf("|----------------------------------------------|\n");

            }
            break;
        }
    }

    close(fd); //closes the serial port
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
    printf("bcc2Field= 0x%02X\n", bcc2Field);
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

int getBaudRate(speed_t baud){
    switch (baud) {
        case B110:
            return 110;
        case B300:
            return 300;
        case B600:
            return 600;
        case B1200:
            return 1200;
        case B2400:
            return 2400;
        case B4800:
            return 4800;
        case B9600:
            return 9600;
        case B19200:
            return 19200;
        case B38400:
            return 38400;
        case B57600:
            return 57600;
        case B115200:
            return 115200;
        case B230400:
            return 230400;
        case B460800:
            return 460800;
        case B500000:
            return 500000;
        case B576000:
            return 576000;
        case B921600:
            return 921600;
        case B1000000:
            return 1000000;
        case B1152000:
            return 1152000;
        case B1500000:
            return 1500000;
        case B2000000:
            return 2000000;
        case B2500000:
            return 2500000;
        case B3000000:
            return 3000000;
        case B3500000:
            return 3500000;
        case B4000000:
            return 4000000;
        default: 
            return -1;
    }
}
