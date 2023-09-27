// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

volatile int STOP = FALSE;

int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    STOP=TRUE;
    printf("Alarm #%d\n", alarmCount);
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

    // Open serial port device for reading and writing, and not as controlling tty
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
    
    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler);

    // Create string to send
    unsigned char buf_write[BUF_SIZE] = {0};

    buf_write[0] = 0x7E;
    buf_write[1] = 0x03;
    buf_write[2] = 0x03;
    buf_write[3] = buf_write[1] ^ buf_write[2];
    buf_write[4] = 0x7E;

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    buf_write[5] = '\n';
    while (alarmCount < 4)
    {
        int bytes = write(fd, buf_write, BUF_SIZE);
        if (alarmEnabled == FALSE)
        {
            alarm(3); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }
        printf("%d bytes written\n", bytes);

        // Wait until all bytes have been written to the serial port
        sleep(1);

        unsigned char buf_read[2] = {0}; // +1: Save space for the final '\0' char
        // Returns after 5 chars have been input
        int count=0;
        while(!STOP) {
            int bytes = read(fd, buf_read, 1);
            if (bytes != 0) {
                printf("%d bytes read\n", bytes);
                printf("buffer= %d\n", buf_read[0]);
            }
            if (count != 0) {
                printf("count= %d\n", count);
            }
            if (count != 4 && buf_read[0] == 0x7E) {
                count = 1;
            }
            else if (count == 1) {
                if (buf_read[0] == 0x7E) {
                    count = 1;
                }
                else if (buf_read[0] == 0x01) {
                    count = 2;
                }
                else count = 0;
            }
            else if (count == 2) {
                if (buf_read[0] == 0x7E)
                    count = 1;
                else if (buf_read[0] == 0x07) {
                    count = 3;
                }
                else count = 0;
            } else if (count == 3) {
                if (buf_read[0] == 0x7E)
                    count = 1;
                else if (buf_read[0] == (0x01 ^ 0x07)) {
                    count = 4;
                } else count = 0;
            } else if (count == 4) {
                if (buf_read[0] == 0x7E) {
                    STOP = TRUE;
                    alarmCount=5;
                    alarmEnabled=FALSE;
                    printf("success");
                }
                else {
                    count = 0;
                }
            }
        }
    }


	//alarm(0);

	//printf("test\n");
	
    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
