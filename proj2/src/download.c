#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include<arpa/inet.h>

typedef enum{
    INITIAL,    //ftp://
    USERNAME,   //username
    PASSWORD,   //password
    HOSTNAME,   //hostname
    PATH        //path
} State;


int getIP(char* hostname, char* ip){
    struct hostent *h;

    if ((h = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }

    *ip = inet_ntoa(*((struct in_addr *) h->h_addr));

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", ip);

    return 0;
}

//          ftp://joao:123@ftp.up.pt/pub
//          ftp://ftp.up.pt/pub   

int parseString(char* string, char* username, char* password, char* hostname, char* path){
    State state = INITIAL;
    int count = 0;
    int usernameCount = 0;
    int passwordCount = 0;
    int hostnameCount = 0;
    int pathCount = 0;
    int hostnameIndex = 0;
    int length = sizeof(string)/sizeof(char);
    while(count < length){
        switch(state){
            case INITIAL:
                if(string[count] == 'f' && string[count+1] == 't' && string[count+2] == 'p' && string[count+3] == ':' && string[count+4] == '/' && string[count+5] == '/'){
                    state = HOSTNAME;
                    count += 6;
                }
                else{
                    printf("Error parsing string\n");
                    return -1;
                }
                break;
            case HOSTNAME:
                if(string[count] == '@'){
                    printf("Current Hostname: %s\n", hostname); //Its actually the username and password
                    printf("Current Hostname Count: %d\n", hostnameCount);
                    hostnameCount = 0;
                    memset(hostname, 0, sizeof(hostname)); //Reset the hostname
                    hostnameIndex = count+1;
                    printf("Hostname Index: %d\n", count);
                    count--;
                    state = PASSWORD;
                }
                else if(string[count] == '/'){
                    printf("Final Hostname: %s\n", hostname);
                    printf("Final Hostname Count: %d\n", hostnameCount);
                    state = PATH;
                    count++;
                }
                else{
                    hostname[hostnameCount] = string[count];
                    count++;
                    hostnameCount++;
                }
                break;
            case PATH:
                path[pathCount] = string[count];
                count++;
                pathCount++;
                break;
            case PASSWORD:
                if(string[count] == ':'){
                    printf("Current Password: %s\n", password);
                    printf("Current Password Count: %d\n", passwordCount);
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
                    printf("Current Username: %s\n", username);
                    printf("Current Username Count: %d\n", usernameCount);
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