#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>


#define FTP_PORT 21
#define PASSIVE_MODE "pasv"

#define DEFAULT_USER "anonymous"
#define DEFAULT_PASSWORD "anonymous"

typedef enum{
    INITIAL,    //ftp://
    USERNAME,   //username
    PASSWORD,   //password
    HOSTNAME,   //hostname
    PATH        //path
} State;

/**
 * @brief Reverses the input string
 * 
 * @param str String to be reversed
 */
void reverse(char *str);

/**
 * @brief Get the Status Code of the response
 * 
 * @param str The response string to be parsed
 * @return char* Status code
 */
char* getStatusCode(const char* str);

/**
 * @brief Returns the IP address of the hostname
 * 
 * @param hostname String containing the hostname
 * @return char* IP address of the hostname
 */
char* getIP(char* hostname);

/**
 * @brief Returns the IP and port of the server
 * 
 * @param response The string containing the pasv response from the server
 * @param ip The IP of the server
 * @param dataSocketPort The port of the server
 * @return int 0 if success, -1 otherwise
 */
int parsePassiveResponse(const char* response, char* ip, int* dataSocketPort);

/**
 * @brief Parses the input string and returns the username, password, hostname, path and filename
 * 
 * @param string Input string
 * @param username Username
 * @param password Password
 * @param hostname Hostname
 * @param path URL path
 * @param filename Name of the file to be downloaded
 * @return int 0 if success, -1 otherwise
 */
int parseString(char string[], char username[], char password[], char hostname[], char path[], char filename[]);

/**
 * @brief Connects to the server
 * 
 * @param ip IP of the server
 * @param port Port of the server
 * @return int ID of the socket
 */
int connectToServer(char* ip, int port);

/**
 * @brief Closes the socket
 * 
 * @param sockfd ID of the socket
 * @return int 0 if success, -1 otherwise
 */
int closeSockets(int controlSocket, int dataSocket);

/**
 * @brief Writes a message to the server
 * 
 * @param sockfd ID of the socket to be written to
 * @param message Message to be written
 * @return int 0 if success, -1 otherwise
 */
int writeToServer(int sockfd, char* message);

/**
 * @brief Reads information from the server
 * 
 * @param sockfd ID of the socket to read information from
 * @param message Information read from the server
 * @return int 0 if success, -1 otherwise
 */
int readFromServer(int sockfd, char* message);

/**
 * @brief Prints a progress indicator for user feedback
 * 
 * @param chunksReceived Number of chunks received
 */
void printProgressIndicator(int chunksReceived);

/**
 * @brief Reads the file from the socket and writes to a local file
 * 
 * @param dataSocket ID of the data socket to read from
 * @param filename Name of the local file to create and write to
 * @return int total number of bytes read
 */
int downloadFile(int dataSocket, char* filename);
