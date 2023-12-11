#include "download.h"


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

int parsePassiveResponse(const char* response, char* ip, int* dataSocketPort) {
    int values[6];
    sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", 
           &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);

    sprintf(ip, "%d.%d.%d.%d", values[0], values[1], values[2], values[3]);
    *dataSocketPort = values[4] * 256 + values[5];
    return 0;
}

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
    printf("Path: %s\n", path);
    printf("Filename: %s\n", filename);
    return 0;
}

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

int closeSockets(int controlSocket, int dataSocket){
    if (close(controlSocket)<0) {
        perror("Error closing control socket");
        exit(-1);
    }
    if (close(dataSocket)<0) {
        perror("Error closing data socket");
        exit(-1);
    }
    return 0;
}

int writeToServer(int sockfd, char* message){
    
    size_t bytes;

    bytes = write(sockfd, message, strlen(message));
    if (bytes <= 0){
        perror("Error writing to socket");
        exit(-1);
    }
    return 0;
}

int readFromServer(int sockfd, char* message){
    size_t bytes;

    bytes = read(sockfd, message, 1000);
    if (bytes <= 0){
        perror("Error reading from socket");
        exit(-1);
    }
    message[bytes] = '\0';
    return 0;
}

long getFileSize(char* response) {
    long size;
    char* start = strchr(response, '(');
    if (start != NULL && sscanf(start, "(%ld bytes)", &size) == 1) {
        return size;
    } else {
        fprintf(stderr, "Could not parse file size from response\n");
        return -1;
    }
}

char* getServerResponse(int sockfd, char* message, char* response){
    printf("Sending message: %s\n", message);
    writeToServer(sockfd, message);
    readFromServer(sockfd, response);
    printf("Server response: %s\n\n", response);
    return getStatusCode(response);
}

void printProgressBar(long totalBytes, long fileSize) {
    int percentage = (int)((100.0 * totalBytes) / fileSize);

    printf("\r[");
    for (int i = 0; i < 100; i += 2) {
        if (i < percentage) {
            printf("#");
        } else {
            printf(" ");
        }
    }
    printf("] %d%%", percentage);
    fflush(stdout);
}

int downloadFile(int dataSocket, char *filename, long fileSize){   
    
    FILE *fp = fopen(filename, "wb");
    if(fp == NULL){
        printf("Error opening file\n");
        exit(1);
    }

    char file[1000];
    int bytes;
    int totalBytes = 0;

    while((bytes = read(dataSocket, file, 1000)) > 0){
        totalBytes += bytes;
        if(fwrite(file, bytes, 1, fp) < 0){
            printf("Error writing to file\n");
            exit(1);
        }
        printProgressBar(totalBytes, fileSize);
    }
    printf("\n");
    fclose(fp);
    return totalBytes;
}

void printDownloadInformation(struct timeval start, int totalBytes) {
    struct timeval end;
    long seconds, useconds;    
    double mtime;

    gettimeofday(&end, NULL);
    printf("\nFinished downloading!\n\n");

    seconds = end.tv_sec - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;
    mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;

    printf("Downloading time: %f seconds\n", mtime/1000.0);
    printf("Download speed: %f MB/s\n", (totalBytes / 1048576.0) / (mtime / 1000.0));
}


int main(int argc, char *argv[]){
    printf("\n--------- INPUT INFORMATION ---------\n\n");
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

    //Retrieves the IP of the hostname
    char* ip;
    ip = getIP(hostname);

    // This opens a control socket and connects to the server
    int controlSocket = connectToServer(ip, FTP_PORT);
    printf("\n-------------------------------------\n\n");

    char message[100];
    char response[1000];
    char* statusCode;

    readFromServer(controlSocket, response);
    printf("Connect response: %s\n\n", response);
    statusCode = getStatusCode(response);

    if(strcmp(statusCode, TELNET_RESPONSE) != 0){
        printf("Error connecting to server\n");
        exit(1);
    }

    memset(response, 0, 1000);
    memset(message, 0, 100);
    sprintf(message, USER_MESSAGE, username);
    if(strcmp(getServerResponse(controlSocket, message, response), USER_RESPONSE) != 0){
        printf("Error sending username\n");
        exit(1);
    }

    memset(response, 0, 1000);
    memset(message, 0, 100);
    sprintf(message, PASS_MESSAGE, password);
    if(strcmp(getServerResponse(controlSocket, message, response), PASS_RESPONSE) != 0){
        printf("Error sending password\n");
        exit(1);
    }

    memset(response, 0, 1000);
    memset(message, 0, 100);
    sprintf(message, PASSIVE_MODE);
    if(strcmp(getServerResponse(controlSocket, message, response), PASV_RESPONSE) != 0){
        printf("Error sending passive mode\n");
        exit(1);
    }

    char dataSocketIP[100];
    int dataSocketPort = 0;
    parsePassiveResponse(response, dataSocketIP, &dataSocketPort);

    // This opens the data socket
    int dataSocket = connectToServer(dataSocketIP, dataSocketPort);

    // This writes the message to retrieve the file
    memset(response, 0, 1000);
    memset(message, 0, 100);
    sprintf(message, RETRIEVE_MESSAGE, path);
    if(strcmp(getServerResponse(controlSocket, message, response), RETR_RESPONSE) != 0){
        printf("Error sending retrieve message\n");
        exit(1);
    }

    long fileSize = getFileSize(response);
    struct timeval start;
    gettimeofday(&start, NULL);
    printf("Downloading file...\n");

    int totalBytes = downloadFile(dataSocket, filename, fileSize);

    printDownloadInformation(start, totalBytes);

    memset(response, 0, 1000);
    readFromServer(controlSocket, response);
    printf("\nServer response: %s", response);
    statusCode = getStatusCode(response);

    if(strcmp(statusCode, TRANSFER_COMPLETE) != 0){
        printf("Error transfering file\n");
        exit(1);
    }
    
    // This closes the sockets
    closeSockets(controlSocket, dataSocket);

    return 0;
}