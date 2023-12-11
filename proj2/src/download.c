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

struct timeval start, end;
long seconds, useconds;
double mtime;

typedef enum{
    INITIAL,    //ftp://
    USERNAME,   //username
    PASSWORD,   //password
    HOSTNAME,   //hostname
    PATH        //path
} State;

// Reverses a string
void reverse(char *str) {
    int i, j;
    char temp;
    for (i = 0, j = strlen(str) - 1; i < j; i++, j--) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

char* getStatusCode(const char* str) {
    char* result = malloc(4);
    strncpy(result, str, 3);
    result[3] = '\0';
    return result;
}

// Gets the IP of a hostname
char* getIP(char* hostname){
    struct hostent *h;

    if ((h = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }

    char* ip = inet_ntoa(*((struct in_addr *) h->h_addr));

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", ip);

    return ip;
}

// Returns the IP and port of the data socket
int parsePassiveResponse(const char* response, char* ip, int* dataSocketPort) {
    int values[6];
    sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", 
           &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);

    sprintf(ip, "%d.%d.%d.%d", values[0], values[1], values[2], values[3]);
    *dataSocketPort = values[4] * 256 + values[5];
    return 0;
}

// Parses the string given by the user
int parseString(char string[], char username[], char password[], char hostname[], char path[], char filename[]){
    State state = INITIAL;
    int count = 0;
    int usernameCount = 0;
    int passwordCount = 0;
    int hostnameCount = 0;
    int pathCount = 0;
    int filenameCount = 0;
    int hostnameIndex = 0;
    int length = 0;

    for (int i = 0; string[i] != '\0'; i++){
        length++;
    }

    while(count < length){
        switch(state){
            case INITIAL:
                if(string[count] == 'f' && string[count+1] == 't' && string[count+2] == 'p' && string[count+3] == ':' && string[count+4] == '/' && string[count+5] == '/'){
                    state = HOSTNAME;
                    count += 6;
                }
                else{
                    printf("Error parsing string: %s\n", string);
                    return -1;
                }
                break;
            case HOSTNAME:
                if(string[count] == '@'){
                    memset(hostname, 0, hostnameCount); //Reset the hostname
                    hostnameCount = 0;
                    hostnameIndex = count+1;
                    count--;
                    state = PASSWORD;
                }
                else if(string[count] == '/'){
                    state = PATH;
                    count++;
                }
                else{
                    hostname[hostnameCount] = string[count];
                    hostnameCount++;
                    count++;
                }
                break;
            case PATH:
                path[pathCount] = string[count];
                if(string[count] == '/'){
                    filenameCount = 0;
                    memset(filename, 0, filenameCount); //Reset the filename
                }
                else{
                    filename[filenameCount] = string[count];
                    filenameCount++;
                }
                count++;
                pathCount++;
                break;
            case PASSWORD:
                if(string[count] == ':'){
                    count--;
                    state = USERNAME;
                }
                else{
                    password[passwordCount] = string[count];
                    count--;
                    passwordCount++;
                }
                break;
            case USERNAME:
                if(string[count] == '/'){
                    state = HOSTNAME;
                    count = hostnameIndex;
                }
                else{
                    username[usernameCount] = string[count];
                    count--;
                    usernameCount++;
                }
                break;     
        }
    }


    username[usernameCount] = '\0';
    password[passwordCount] = '\0';
    hostname[hostnameCount] = '\0';
    path[pathCount] = '\0';

    reverse(username);
    reverse(password);

    //If no username or password is given, use anonymous
    if(usernameCount == 0 && passwordCount == 0){
        printf("No username or password given\n");
        sprintf(username, DEFAULT_USER);
        sprintf(password, DEFAULT_PASSWORD);
    }

    printf("Username: %s\n", username);
    printf("Password: %s\n", password);
    printf("Hostname: %s\n", hostname);
    printf("Path: %s\n", path);
    printf("Filename: %s\n", filename);
    return 0;
}

// Opens a socket and connects to the server
int connectToServer(char* ip, int port){
    int sockfd;
    struct sockaddr_in server_addr;
    
    /*server address handling*/
    bzero((char*)&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);	/*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);		/*server TCP port must be network byte ordered */

    /*open an TCP socket*/
    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Error opening socket");
        exit(-1);
    }
    /*connect to the server*/
    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("Error connecting to server");
        exit(-1);
    }

    return sockfd;
}

// Closes a socket
int closeSocket(int sockfd){
    if (close(sockfd)<0) {
        perror("Error closing socket");
        exit(-1);
    }
    return 0;
}

// Writes a message to the server
int writeToServer(int sockfd, char* message){
    
    size_t bytes;

    bytes = write(sockfd, message, strlen(message));
    if (bytes > 0)
        printf("%ld bytes written to socket %d\n", bytes, sockfd);
    else {
        perror("Error writing to socket");
        exit(-1);
    }

    return 0;
}

int readFromServer(int sockfd, char* message){
    size_t bytes;

    bytes = read(sockfd, message, 1000);
    if (bytes > 0)
        printf("%ld bytes read from socket %d\n", bytes, sockfd);
    else {
        perror("Error reading from socket");
        exit(-1);
    }
    return 0;
}

void printProgressIndicator(int chunksReceived) {
    if (chunksReceived % 1000 == 0){
        printf(".");
        fflush(stdout);
    }
}

int main(int argc, char *argv[]){
    printf("Number of arguments: %d\n", argc);
    printf("First argument: %s\n", argv[1]);
    if (argc < 2){
        printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(1);
    }

    char username[100];
    char password[100];
    char hostname[100];
    char path[100];
    char filename[100];

    if(parseString(argv[1], username, password, hostname, path, filename) != 0){
        printf("Error parsing string\n");
        exit(1);
    }

    char* ip;

    //Retrieves the IP of the hostname
    ip = getIP(hostname);
    
    // This opens a control socket and connects to the server
    int controlSocket = connectToServer(ip, FTP_PORT);

    char userMessage[100];
    sprintf(userMessage, "USER %s\n", username);
    char passwordMessage[100];
    sprintf(passwordMessage, "PASS %s\n", password);
    char passiveMessage[100];
    sprintf(passiveMessage, "%s\n", PASSIVE_MODE);

    char connectResponse[1000];
    char userResponse[1000];
    char passwordResponse[1000];
    char passiveResponse[3000];

    char* statusCode;

    sleep(1);
    readFromServer(controlSocket, connectResponse);
    printf("Connect response: %s\n", connectResponse);
    statusCode = getStatusCode(connectResponse);
    if(strcmp(statusCode, "220") != 0){
        printf("Error connecting to server\n");
        exit(1);
    }

    printf("Sending username message: %s\n", userMessage);
    writeToServer(controlSocket, userMessage);
    readFromServer(controlSocket, userResponse);
    printf("User response: %s\n", userResponse);
    statusCode = getStatusCode(userResponse);
    if(strcmp(statusCode, "331") != 0){
        printf("Error sending username\n");
        exit(1);
    }

    printf("Sending password: %s\n", passwordMessage);
    writeToServer(controlSocket, passwordMessage);
    readFromServer(controlSocket, passwordResponse);
    printf("Pass response: %s\n", passwordResponse);
    statusCode = getStatusCode(passwordResponse);
    if(strcmp(statusCode, "230") != 0){
        printf("Error sending password\n");
        exit(1);
    }

    printf("Sending passive message: %s\n", passiveMessage);
    writeToServer(controlSocket, passiveMessage);
    readFromServer(controlSocket, passiveResponse);
    printf("Pasv response: %s\n", passiveResponse);
    statusCode = getStatusCode(passiveResponse);
    if(strcmp(statusCode, "227") != 0){
        printf("Error entering passive mode\n");
        exit(1);
    }


    char dataSocketIP[100];
    int dataSocketPort = 0;
    parsePassiveResponse(passiveResponse, dataSocketIP, &dataSocketPort);

    // This opens the data socket
    int dataSocket = connectToServer(dataSocketIP, dataSocketPort);

    // This writes the message to retrieve the file
    char* pathMessage = malloc(100);
    sprintf(pathMessage, "retr %s\n", path);
    writeToServer(controlSocket, pathMessage);


    // This writes the file information to a local file
    FILE *fp = fopen(filename, "wb");
    if(fp == NULL){
        printf("Error opening file\n");
        exit(1);
    }

    // This reads the file information from the data socket and writes it to the local file
    char file[1000];
    int bytes;
    int chunksReceived = 0;
    printf("Downloading file...\n");
    gettimeofday(&start, NULL);
    while((bytes = read(dataSocket, file, 1000)) > 0){
        chunksReceived++;
        printProgressIndicator(chunksReceived);
        if(fwrite(file, bytes, 1, fp) < 0){
            printf("Error writing to file\n");
            exit(1);
        }
    }
    printf("\n");
    fclose(fp);
    gettimeofday(&end, NULL);
    printf("Finished  downloading!\n");
    seconds = end.tv_sec - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;

    mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
    printf("Downloading time: %f seconds\n", mtime/1000.0);

    // This closes the sockets
    closeSocket(dataSocket);
    closeSocket(controlSocket);

    return 0;
}