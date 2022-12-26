#include "download.h"

struct urlData data;
int fd;
int newfd;


int main(int argc, char *argv[]) {

    if(argc < 2){
       fprintf(stderr, "Incorrect usage! Try: ftp://<host>/<url-path> or ftp://<user>:<password>@<host>/<url-path>\n");
       exit(-1);
    }

    char* str = strdup(argv[1]);
    argParser(str, &data);

    fd = startSocket(data.ip,21);

    if(readMessage(fd) != SOCK_READY) {
        printf("Error opening Socket");
        return -1;
    }

    if(login(data.user,data.password,fd) != 0) {
        printf("Error Login");
        return -1;
    }

     if(passiveMode(fd) != 0) {
        printf("Error Passive");
        return -1;
    }

    newfd = startSocket(data.ip,data.porta);

    if(fileMessage(fd) != 0) {
        printf("Error getting file");
        return -1;
    }

    if(writeFile(newfd) != 0) {
        printf("Error saving file");
        return -1;
    }


    close(fd);
    close(newfd);
    return 0;
}

//ftp://<user>:<password>@<host>/<url-path> or ftp://<host>/<url-path>
int argParser(char* input, struct urlData* data) {
    struct hostent *h;
    char* type;
    type = strtok(input, ":");

    if(strcmp(type,"ftp") != 0 || type == NULL) {
        printf("Protocol isn't FTP!");
        return -1;
    }

    char* res = strtok(NULL,"\0");

    // means there is no username/pw
    if (strchr(res, '@') == NULL)
    {
        char* host_name = strtok(res,"/");
        strcpy(data->host_name,host_name);
        printf("Host: %s\n",host_name);

        strcpy(data->user,"anonymous");

        strcpy(data->password,"");

        char* path = strtok(NULL,"");
        strcpy(data->file_path,path);
        printf("Path: %s\n",path);

        char* name = strrev(strtok(strrev(path),"/"));

        printf("FileName: %s\n",name);
        strcpy(data->file_name,name);

        data->porta = 21;
    }
    else {
        char* user = strtok(res,":") + 2;
        strcpy(data->user,user);
        printf("User: %s\n",user);

        char* pw = strtok(NULL,"@");
        strcpy(data->password,pw);
        printf("PW: %s\n",pw);

        char* host_name = strtok(NULL,"/");
        strcpy(data->host_name,host_name);
        printf("Host: %s\n",host_name);

        char* path = strtok(NULL,"");
        strcpy(data->file_path,path);
        printf("Path: %s\n",path);

        char* name = strrev(strtok(strrev(path),"/"));
        printf("FileName: %s\n",name);
        strcpy(data->file_name,name);

        data->porta = 21;

    }

    if ((h = gethostbyname(data->host_name)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }
    strcpy(data->ip,inet_ntoa(*((struct in_addr *) h->h_addr)));
    //printf("%s\n",data->ip);
    return 0;
}

int startSocket(char* ip,int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";
    size_t bytes;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);   
    server_addr.sin_port = htons(port);        

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    /*connect to the server*/
    if (connect(sockfd,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
        perror("connect()");
        return -1;
    }

    return sockfd;
}

int readMessage(int fd) {
    int code;
    FILE * sock = fdopen(fd,"r");

    char * buf;
    size_t currBytes = 0;


    while(getline(&buf,&currBytes,sock) > 0) {
        printf("%s",buf);
        if(buf[3] == ' ') {
            sscanf(buf, "%d", &code);
	    break;
        }
    }

    printf("CODE: %d\n",code);

    return code;
}

int sendMessage(int fd, char* msg) {
    int st = send(fd,msg,strlen(msg),0);
    printf("Sent: %s",msg);
    if(st <= 0) {
        printf("Error Sending Message");
        return -1;
    }
    return 0;
}

int login(char* username, char* password, int fd) {
    char msg[200];
    sprintf(msg,"user %s\r\n", username);
    if(sendMessage(fd,msg) != 0) {
        printf("Error in Login Username");
        return -1;
    }
    if(readMessage(fd) != 331) {
        //printf("Error in Login Username");
        //return -1;
    };
    memset(&msg,0,sizeof(msg));
    sprintf(msg,"pass %s\r\n", password);
    if(sendMessage(fd,msg) != 0) {
        printf("Error in Login Password");
        return -1;
    }
    if(readMessage(fd) != 230) {
        printf("BAD PASSWORD");
        return -1;
    };
    return 0;
}

int readPassive(int fd) {
    int code;
    FILE * sock = fdopen(fd,"r");

    char * buf;
    size_t currBytes = 0;


    while(getline(&buf,&currBytes,sock) > 0) {
        printf("%s",buf);
        if(buf[3] == ' ') {
            sscanf(buf, "%d", &code);
			break;
        }
    }

    char* total = strtok(buf,"(");
    char* rest = strtok(NULL,""); // ip todo
    char* pt1,*pt2,*pt3,*pt4;
    char* porta1,*porta2;

    pt1 = strtok(rest,",");
    pt2 = strtok(NULL,",");
    pt3 = strtok(NULL,",");
    pt4 = strtok(NULL,",");

    porta1 = strtok(NULL,",");
    porta2 = strtok(NULL,")");

    data.porta = atoi(porta1)*256 + atoi(porta2);
    sprintf(data.ipNovo, "%s.%s.%s.%s", pt1, pt2, pt3, pt4);

    return 0;
    
}

int passiveMode(int fd) {
    char msg[200];
    sprintf(msg,"pasv\r\n");
    if(sendMessage(fd,msg) != 0) {
        printf("Error in passive mode");
        return -1;
    }

    readPassive(fd);
    return 0;
}

int fileMessage(int fd) {
    char msg[250];
    sprintf(msg,"retr %s\r\n",data.file_path);
    if(sendMessage(fd,msg) != 0) {
        printf("Error in file");
        return -1;
    }

    if(readMessage(fd) != 150) {
        printf("Error in File");
        return -1;
    }

    return 0;
}

int writeFile(int fd) {
    int fp;
    if ((fp = open(&data.file_name, O_WRONLY | O_CREAT, 0777)) < 0) {
        printf("Error opening file\n");
        return -1;
    }

    char buf[256];
    int numBytesRead;

    while((numBytesRead = read(newfd, buf, 256)) > 0) {
        if (write(fp, buf, numBytesRead) < 0) {
            printf("Error writing to file!\n");
            return -1;
        }
    }

    if (close(fp) < 0) {
        printf("Error closing file\n");
        return -1;
    }

    return 0;
}

char *strrev(char *str)
{
      char *p1, *p2;

      if (! str || ! *str)
            return str;
      for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
      {
            *p1 ^= *p2;
            *p2 ^= *p1;
            *p1 ^= *p2;
      }
      return str;
}
