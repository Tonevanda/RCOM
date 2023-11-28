#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>

typedef enum{
    INITIAL,    //ftp://
    USERNAME,   //username
    PASSWORD,   //password
    HOSTNAME,   //hostname
    PATH        //path
} State;


void reverse(char *str) {
    int i, j;
    char temp;
    for (i = 0, j = strlen(str) - 1; i < j; i++, j--) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

int getIP(char* hostname, char* ip){
    struct hostent *h;

    if ((h = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }

    ip = inet_ntoa(*((struct in_addr *) h->h_addr));

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", ip);

    return 0;
}
 

int parseString(char string[], char username[], char password[], char hostname[], char path[]){
    State state = INITIAL;
    int count = 0;
    int usernameCount = 0;
    int passwordCount = 0;
    int hostnameCount = 0;
    int pathCount = 0;
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
        username = "anonymous";
        password = "anonymous";
    }

    printf("Username: %s\n", username);
    printf("Password: %s\n", password);
    printf("Hostname: %s\n", hostname);
    printf("Path: %s\n", path);
    return 0;
}

int main(int argc, char *argv[]){
    printf("Number of arguments: %d\n", argc);
    printf("First argument: %s\n", argv[1]);
    if (argc < 2){
        printf("Usage: %s hostname\n", argv[0]);
        exit(1);
    }

    char username[100];
    char password[100];
    char hostname[100];
    char path[100];

    if(parseString(argv[1], username, password, hostname, path) != 0){
        printf("Error parsing string\n");
        exit(1);
    }

    char ip[100];

    //Retrieves the IP of the hostname
    getIP(hostname, ip);

    return 0;
}