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
#include <sys/time.h>
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
    int nOfREJSent;
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
#define MAX_PAYLOAD_SIZE 300

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B19200
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

#define T_PROP 0.1
#define PROBABILITY 0

int getBaudRate(speed_t baud);


// Abre a conexão entre os dois computadores utilizando os parâmetros definidos na struct linkLayer.
// Retorna 1 em caso de sucesso e -1 em caso de erro.
int llopen(LinkLayer connectionParameters);

// Envia os dados contidos no buffer com o tamanho bufSize.
// Retorna o número de bytes escritos ou -1 em caso de erro.
int llwrite(const unsigned char *buf, int bufSize);

// Lê os dados contidos no pacote a ser recebido.
// Retorna o número de bytes lidos ou -1 em caso de erro ou -2 caso tenha lido um DISC.
int llread(unsigned char *packet);

// Fecha a conexão entre os dois computadores.
// Se showStatistics == TRUE, a camada de ligação deve imprimir as estatisticas no terminal.
// Retorna 1 em caso de sucesso e -1 em caso de erro.
int llclose(int showStatistics);

// Faz a conexão entre os dois computadores (chamado pelo llopen)
// Retorna fd ou -1 em caso de erro.
int connect(const char *serialPort);

// Lida com a interrupção do alarme
void alarmHandler(int signal);

// Escreve o pacote de supervisão para o recetor
// Retorna o número de bytes escritos
int writeSupervisionFrame(unsigned char A, unsigned char C);

// Realiza Byte Stuffing no buffer
int stuffBytes(unsigned char *buf_write, int *bufSize, const unsigned char *buf);

int changeFrame(unsigned char * frame, int size, int probability, int * idx);

int changeFrameBack(unsigned char * frame, int idx, int saved);

#endif // _LINK_LAYER_H_
