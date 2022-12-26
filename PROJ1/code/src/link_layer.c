// Link layer protocol implementation

#include "link_layer.h"
#include "utils.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
LinkLayer str;
enum stateMachine stOpen = START;
enum stateMachine stWrite = START;
enum stateMachine stRead = START;
enum stateMachine stClose = START;
struct termios oldtio;
struct termios newtio;
int fd;
int ns = 0;
int nr = 1;
int valid = TRUE;
int alarmEnabled = FALSE;
int alarmCount = 0;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    printf("Alarm #%d\n", alarmCount);
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

int resetAlarm() {
    alarmCount = 0;
    alarmEnabled = FALSE;
}

int openSP() {
    fd = open(str.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0) {
        printf("error");
        return -1;
    }

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1) {
        printf("error2");
        return -1;
    }

    memset(&newtio, 0, sizeof(newtio));
    newtio.c_cflag = str.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0.1; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Read without blocking
    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
        return -1;

    return fd;
}

int sendSet() {
    unsigned char buf[BUF_SIZE + 1] = {0};
    buf[0] = FLAG;
    buf[4] = FLAG;
    buf[1] = TRANS;
    buf[2] = SET;
    buf[3] = buf[1] ^ buf[2];
    (void)signal(SIGALRM, alarmHandler);

    while (stOpen != STOP && alarmCount < (str.nRetransmissions + 1))
    {
        if (!alarmEnabled)
        {
            printf("Set Written\n");
            stOpen = START;
            write(fd, buf, 5);
            alarm(str.timeout); 
            alarmEnabled = TRUE;
        }

        if (read(fd, buf, 1) > 0) {
            changeState(&stOpen,buf[0],0);
            if(stOpen == STOP)
            {
                printf("Read UA\n");
                break;
            }
        }
    }
    return alarmCount;
}

int readSetSendUA() {
    unsigned char buf[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char
    unsigned int n = 0;
    while (TRUE)
    {
        // Returns after 5 chars have been input
        int bytes = read(fd, buf, 1); // em bytes vai ficar o tamanho da string
                                         // pode ser por um char especial no fim do write, para saber quando parar de ler
        

        changeState(&stOpen,buf[0],0);
        if (stOpen == STOP){ // Set end of string to '\0', so we can printf
            printf("Read SET\n");
            break;
        }
   }

    buf[0] = FLAG;
    buf[4] = FLAG;
    buf[1] = TRANS;
    buf[2] = UA;
    buf[3] = buf[1] ^ buf[2];
    int bytes = write(fd, buf, 5);
    printf("Sent UA\n");
    return 1;
}

int llopen(LinkLayer connectionParameters)
{   
    str = connectionParameters;
    openSP();
    switch (str.role)
    {
    case LlTx: // write set & read UA
        sendSet();
        break;
    
    case LlRx: // read set & send UA
        readSetSendUA();
        break;
    default:
        break;
    }
    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    unsigned char info[2 * MAX_PAYLOAD_SIZE];

    // montar 4 bytes iniciais
    info[0] = FLAG;
    info[1] = TRANS;
    info[2] = (ns == 0) ? 0x00 : 0x40;
    info[3] = BCC(info[1],info[2]);

    int infoSz = 0;
    int bcc_2 = 0;
    bcc_2 ^= buf[0];
    bcc_2 ^= buf[1];
    bcc_2 ^= buf[2];
    bcc_2 ^= buf[3];

    for(int i = 4; i < bufSize; i++) {
        bcc_2 ^= buf[i];
    }
    // montar os dados
    for(int i = 0; i < bufSize; i++) {
        if(buf[i] == FLAG) {
            info[4+infoSz] = ESC;
            info[5+infoSz] = ESCE;
            infoSz+= 2;
        }
        else if(buf[i] == ESC) {
            info[4+infoSz] = ESC;
            info[5+infoSz] = ESCD;
            infoSz+= 2;
        }
        else {
            info[4+infoSz] = buf[i];
            infoSz++;
        }
    }
    if(bcc_2 == FLAG) {
        info[4+infoSz] = ESC;
        info[5+infoSz] = ESCE;
        info[6+infoSz] = FLAG;
        infoSz+= 1;
    }
    else if(bcc_2 == ESC) {
        info[4+infoSz] = ESC;
        info[5+infoSz] = ESCD;
        info[6+infoSz] = FLAG;
        infoSz+= 1;
    }
    else {
        info[4+infoSz] = bcc_2;
        info[5+infoSz] = FLAG;
    }

    infoSz += 6;

    unsigned char bufi[BUF_SIZE + 1] = {0};
    (void)signal(SIGALRM, alarmHandler);

    while (stWrite != STOP && alarmCount < (str.nRetransmissions + 1))
    {
        if (!alarmEnabled || stWrite == REJ)
        {

            stWrite = START;
            fprintf(stderr, "Sent Packet Sent Type: %d\n", ns);
            int b_written = write(fd,info,infoSz);
            alarm(str.timeout); 
            alarmEnabled = TRUE;
        }
        int i = 0;
        unsigned char tmp;
        if (read(fd, bufi + i, 1) > 0) {
            //fprintf(stderr,"state: %d, Buf: %02x \n", stWrite,bufi[i]);
            //fprintf(stderr,"STate: %d", stWrite);
            changeState(&stWrite,bufi[i],0);
            if(stWrite == REJ){
                read(fd, bufi + i, 1);
                i++;
                read(fd, bufi + i, 1);
                i++;
            }
            //fprintf(stderr,"Buf: %02x \n", bufi[i]);
            i++;
            if(stWrite == STOP)
            {
                printf("Read ACK\n");
                ns = (1 + ns) % 2;
                valid = TRUE;
                break;
            }
        }
    }
   

    stWrite = START;
	alarmEnabled = FALSE;
    if(alarmCount >= (str.nRetransmissions + 1)){
        alarmCount = 0;
        llclose(0);
        exit(1);
    }
    return infoSz;
}


////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////

int writeBadPacket(){
    unsigned char outbuf[6];
    outbuf[0] = FLAG;
    outbuf[4] = FLAG;
    outbuf[1] = TRANS;
    if(nr) {
        outbuf[2] = REJ_1;
    }
    else {
        outbuf[2] = REJ_0;
    }
    outbuf[3] = outbuf[1] ^ outbuf[2];
    write(fd, outbuf, 5);
    return 5;
}

int llread(unsigned char *packet)
{
    unsigned char buf[MAX_PAYLOAD_SIZE * 2];
    unsigned char initPacket[MAX_PAYLOAD_SIZE]; // com byte stuffing

    int sz = 0;
    stRead = START;

    memset(packet,0,sizeof(packet));

    while(1) {
        if(read(fd,buf + sz,1) > 0) {
            if(sz+1>MAX_PAYLOAD_SIZE * 2){
                fprintf(stderr,"END PACKET FLAG NOT FOUND: NOISE ON SEREAL PORT\n");
                writeBadPacket();
                return -1;
            }
            changeState(&stRead,buf[sz],1);
            sz++;
            if(stRead == STOP) {
                //printf("Read All\n");
                break;
            }
        }
    }

    stRead = START;

    int bcc_2 = 0;
    if(buf[sz - 3] == ESC && buf[sz - 2] == ESCD) {
        bcc_2 = ESC;
        sz--;
        
    }
    else if(buf[sz - 3] == ESC && buf[sz - 2] == ESCE) {
        bcc_2 = FLAG;
        sz--;
        
    }else{
        bcc_2 = buf[sz-2];  
    }


    for(int i = 0; i < sz - 6; i++) {
        initPacket[i] = buf[4+i];
    }

    

    packet[0] = initPacket[0];
    packet[1] = initPacket[1];
    packet[2] = initPacket[2];
    packet[3] = initPacket[3];

    int j = 4;
    for(int i = 4; i < sz - 6; i++) {
        if(initPacket[i] == ESC && initPacket[i+1] == ESCE) {
            packet[j] = FLAG;
            j++;
            i++;
        }
        else if(initPacket[i] == ESC && initPacket[i+1] == ESCD) {
            packet[j] = ESC;
            j++;
            i++;
        }
        else {
            packet[j] = initPacket[i];
            j++;
        }
    }

    int calBCC = 0;
    calBCC ^= packet[0];
    calBCC ^= packet[1];
    calBCC ^= packet[2];
    calBCC ^= packet[3];
    for(int i = 4; i < j; i++) {
        calBCC ^= packet[i];
    }

    //memset(buf,0,sizeof(buf));
    unsigned char outbuf[6];
    outbuf[0] = FLAG;
    outbuf[4] = FLAG;
    outbuf[1] = TRANS;

    
    if(calBCC == bcc_2){
        nr = (nr + 1) % 2;
        if(nr) {
            outbuf[2] = RR_1;
        }
        else {
            outbuf[2] = RR_0;
        }
    }else if(calBCC != bcc_2){
        if(nr) {
            outbuf[2] = REJ_1;
        }
        else {
            outbuf[2] = REJ_0;
        }
        printf("bad packet\n");
        outbuf[3] = outbuf[1] ^ outbuf[2];
        write(fd, outbuf, 5);
        return -1;
    }

    outbuf[3] = outbuf[1] ^ outbuf[2];

    int bytes = write(fd, outbuf, 5);
    
    fprintf(stderr,"send\n");

    int sz_final = j;
    return sz_final;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////

int sendDiscTransmisser() {
    unsigned char buf[BUF_SIZE + 1] = {0};
    buf[0] = FLAG;
    buf[4] = FLAG;
    buf[1] = TRANS;
    buf[2] = DISC;
    buf[3] = buf[1] ^ buf[2];
    (void)signal(SIGALRM, alarmHandler);

    while (stClose != STOP && alarmCount < (str.nRetransmissions + 1))
    {
        if (!alarmEnabled)
        {
            printf("Disc Written\n");
            stClose = START;
            write(fd, buf, 5);
            alarm(str.timeout); 
            alarmEnabled = TRUE;
        }

        if (read(fd, buf, 1) > 0) {
            changeState(&stClose,buf[0],0);
            if(stClose == STOP)
            {
                printf("Read Disc\n");
                break;
            }
        }
    }
    return alarmCount;
}

int readAndSendDisc() {
    unsigned char buf[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char
    while (TRUE)
    {
        // Returns after 5 chars have been input
        int bytes = read(fd, buf, 1); // em bytes vai ficar o tamanho da string
                                         // pode ser por um char especial no fim do write, para saber quando parar de ler
        

        changeState(&stClose,buf[0],0);
        if (stClose == STOP){ // Set end of string to '\0', so we can printf
            printf("Read Disc\n");
            break;
        }
    }
    buf[0] = FLAG;
    buf[4] = FLAG;
    buf[1] = RECEIVE;
    buf[2] = DISC;
    buf[3] = buf[1] ^ buf[2];
    int bytes = write(fd, buf, 5);
    printf("Send Disc\n");

    return 1;
}

int sendUATransmisser() {
    unsigned char buf[BUF_SIZE + 1] = {0};
    buf[0] = FLAG;
    buf[4] = FLAG;
    buf[1] = RECEIVE;
    buf[2] = UA;
    buf[3] = buf[1] ^ buf[2];
    write(fd, buf, 5);
    printf("Sent UA");
}

int readUA() {
    unsigned char buf[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char
    unsigned int n = 0;
    while (TRUE)
    {
        // Returns after 5 chars have been input
        int bytes = read(fd, buf+n, 1); // em bytes vai ficar o tamanho da string
                                         // pode ser por um char especial no fim do write, para saber quando parar de ler
        

        changeState(&stClose,buf[n],0);
        if (stClose == STOP){ // Set end of string to '\0', so we can printf
            printf("Read UA\n");
            break;
        }
        n++;
    }

}

int llclose(int showStatistics)
{
    alarmEnabled = 0;
    alarmCount = 0;
    switch (str.role)
    {
    case LlTx: // write DISC & read DISC
        if(sendDiscTransmisser() == 3) {return -1;}
        sendUATransmisser();
        break;
    
    case LlRx: // read DISC & send DISC
        readAndSendDisc();
        readUA();
        break;
    default:
        break;
    }

    sleep(1);
    
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }


    close(fd);
    return 1;
}
