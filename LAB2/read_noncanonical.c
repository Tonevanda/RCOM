// Read from serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
//
// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

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

#define BUF_SIZE 256

volatile int STOP = FALSE;

typedef enum{
    DISCONNECTING,  //0
    FIRSTFLAG, //1
    A,   //2
    C,   //3
    BCC1, //4
    DATA,  //5
    BCC2,  //6
    SUCCESS,   //7
    FAILURE,  //8
    FINALFLAG  //9
} State; //expected field


int llopen(int fd){
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
}

int llread(int fd,unsigned char output[BUF_SIZE]){
    printf("llread start\n\n");
    int responseFrame=0;
    int firstDataByte=0;
    unsigned char frame=0x00;
    State state = FIRSTFLAG;
    while (state != DISCONNECTING){
        unsigned char buf_read[2] = {0}; 
        state = FIRSTFLAG;
        while(!STOP){
            int bytes = read(fd, buf_read, 1);
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
            else if(state == FINALFLAG){
                if(buf_read[0] == FLAG){
                    STOP=TRUE;
                    printf("success");
                    state = SUCCESS;
                }
                else{
                    state = FAILURE;
                    firstDataByte-=3;
                }
                if(frame == DISC){
                    printf("\n\n\nsucses in disconecting\n");
                    state = DISCONNECTING;
                }
            }
            else state = FAILURE;
        }

        if(state==SUCCESS){
            printf("sucsess frame sent\n");
            // Create string to send
            unsigned char buf_write[BUF_SIZE] = {0};

            buf_write[0] = FLAG;
            buf_write[1] = 0x01;
            buf_write[2] = responseFrame;
            buf_write[3] = buf_write[1] ^ buf_write[2];
            buf_write[4] = FLAG;
            buf_write[5] = '\n';

            int bytes = write(fd, buf_write, BUF_SIZE);
            STOP=FALSE;
            // Wait until all bytes have been written to the serial port
            sleep(1);
        }
        else if (state==FAILURE) {
            printf("failure frame sent\n");
            // Create string to send
            unsigned char buf_write[BUF_SIZE] = {0};
            if(responseFrame==RR0){
                responseFrame=REJ1;
            }
            else{
                responseFrame=REJ0;
            }
            buf_write[0] = FLAG;
            buf_write[1] = 0x01;
            buf_write[2] = responseFrame;
            buf_write[3] = buf_write[1] ^ buf_write[2];
            buf_write[4] = FLAG;
            buf_write[5] = '\n';

            int bytes = write(fd, buf_write, BUF_SIZE);
            STOP=FALSE;
            // Wait until all bytes have been written to the serial port
            sleep(1);
        }
    }
    STOP=FALSE;
    printf("\n\nllread success\n\n");
    output[firstDataByte]='\n';
    return 0;
}

int llclose(int fd){
    printf("llclose start\n\n");
    State state = FIRSTFLAG;
    unsigned char buf_read[2] = {0};
    unsigned char buf_write[BUF_SIZE] = {0};
    unsigned char a=0x00;
    unsigned char c=0x00;
    buf_write[0] = FLAG;
    buf_write[1] = 0x01;
    buf_write[2] = DISC;
    buf_write[3] = buf_write[1] ^ buf_write[2];
    buf_write[4] = FLAG;
    buf_write[5] = '\n';
    int bytes = write(fd, buf_write, BUF_SIZE);
    while(!STOP){
        int bytes = read(fd, buf_read, 1);
        if(state != FIRSTFLAG){
            if(bytes != 0){
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
            a=buf_read[0];
        }
        else if(state == C){
            if(a==0x03 && buf_read[0]==DISC || a==0x01 && buf_read[0]==UA){
                c=buf_read[0];
                state = BCC1;
            }
            else state = FIRSTFLAG;
        }
        else if(state == BCC1 && buf_read[0]==(a ^ c)){
            state = FINALFLAG;
        }
        else if(state == FINALFLAG && buf_read[0] == FLAG){
            if(c==UA){
                STOP=TRUE;
                printf("success");
            }
            else{
                int bytes = write(fd, buf_write, BUF_SIZE);
                state=FIRSTFLAG;
            }
        }
        else state = FIRSTFLAG;
    }

    STOP=FALSE;
    printf("\n\nllclose success\n\n");
    return 0;
}

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

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

    llopen(fd);

    unsigned char output[BUF_SIZE]={0};

    llread(fd,output);

    for(int i = 0;i<10;i++){
        printf("output[%d] = 0x%02X\n",i,output[i]);
    }
    llclose(fd);

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
