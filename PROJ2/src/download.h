#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"
#define SOCK_READY 220



struct urlData
{
    char user[200];
    char password[200];
    char host_name[200];
    char file_path[200];
    char file_name[200];
    char ip[25];
    char ipNovo[25];
    int porta;
};


int argParser(char* input, struct urlData* data);

int startSocket(char* ip,int port);

int login(char* username, char* password, int fd);

int sendMessage(int fd, char* msg);

int readMessage(int fd);

int readPassive(int fd);

int passiveMode(int fd);

int fileMessage(int fd);

int writeFile(int fd);

char *strrev(char *str);