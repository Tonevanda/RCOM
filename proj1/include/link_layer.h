// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

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
    int nOfCharDestuffed;

    int nOfBytesllcloseSent;
    int nOfPacketsllcloseSent;
    int nOfBytesllcloseReceived;
    int nOfPacketsllcloseReceived;
} Statistics;

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
} State; //expected field

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

// MISC
#define FALSE 0
#define TRUE 1
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


// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.

//Abre a conecção entre os dois computadores utilizando os parametros definidos na struct linkLayer.
//Retorna 1 em caso de sucesso e -1 em caso de erro.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.

//Envia os dados contidos no buffer com o tamanho bufSize.
//Retorna o numero de bytes escritos ou -1 em caso de erro.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.

//Lê os dados contidos no pacote a ser recebido.
//Retorna o numero de bytes lidos ou -1 em caso de erro
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics);

// Connects to the serialPort passed as parameter and initiates termios structs
// Returns fd if successful or -1 in case of error
int connect(const char *serialPort);

void alarmHandler(int signal);

// Writes the supervision frame to the receiver
// Returns number of bytes written
int writeSupervisionFrame(unsigned char A, unsigned char C);

int stuffBytes(unsigned char *buf_write, int *bufSize, const unsigned char *buf);

#endif // _LINK_LAYER_H_
