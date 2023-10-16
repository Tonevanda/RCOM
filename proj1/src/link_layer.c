// Link layer protocol implementation

#include "link_layer.h"


volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;
int alarmTimeout = 0;
int nRetransmissions = 0;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters){

    printf("llopen start\n\n");

    int fd = connect(connectionParameters.serialPort);
    if(fd != 0) return -1;

    State state = FIRSTFLAG;
    unsigned char buf_read[2] = {0};
    alarmTimeout = connectionParameters.timeout;
    nRetransmissions = connectionParameters.nRetransmissions;

    switch(connectionParameters.role){

        case LlTx: {

            while(nRetransmissions != 0){

                int bytes = writeSupervisionFrame(fd, 0x03, SET);

                if (!alarmEnabled){ // enables the timer with timeout seconds 
                    alarm(alarmTimeout); 
                    alarmEnabled = TRUE;
                }

                printf("%d bytes written\n", bytes);

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
                    } else if (state == BCC1 && buf_read[0] == (0x03 ^ UA)) {
                        state = FINALFLAG;
                    } else if (state == FINALFLAG && buf_read[0] == FLAG) {
                        STOP = TRUE;
                        alarmCount=5;
                        alarmEnabled=FALSE;
                        printf("ff read\n");
                    }
                    else state = FIRSTFLAG;
                }
                nRetransmissions--;
            }

            break;
        }

        case LlRx: {
                /*
                printf("llopen start\n\n");
                unsigned char buf_read[2] = {0};
                State state = FIRSTFLAG;
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
                        printf("success");
                    }
                    else state = FIRSTFLAG;
                }

                // Create string to send
                unsigned char buf_write[BUF_SIZE] = {0};

                buf_write[0] = FLAG;
                buf_write[1] = 0x03;
                buf_write[2] = UA;
                buf_write[3] = buf_write[1] ^ buf_write[2];
                buf_write[4] = FLAG;
                buf_write[5] = '\n';

                int bytes = write(fd, buf_write, BUF_SIZE);
                printf("%d bytes written\n", bytes);

                sleep(1);
                STOP=FALSE;
                printf("\n\nllopen success\n\n");
                return 0;
                */
            break;
        }

    }

    

}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

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
    // TODO

    return 1;
}


int connect(const char *serialPort){
    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
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
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    return fd;
}

void alarmHandler(int signal){
    alarmEnabled = FALSE;
    alarmCount++;
    STOP=TRUE;
    printf("Alarm #%d\n", alarmCount);
}

int writeSupervisionFrame(int fd, unsigned char A, unsigned char C){
    unsigned char supervisionFrame[5] = {FLAG, A, C, A ^ C, FLAG};
    if(write(fd, supervisionFrame, 5) != 0) return -1;
    return 0;
}