// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "utils.h"

extern int valid;
extern int nr;

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer str;
    strcpy(str.serialPort,serialPort);
    str.nRetransmissions = nTries;
    str.timeout = timeout;
    str.baudRate = baudRate;

    if(strcmp(role,"tx") == 0) {    
        str.role = LlTx;
    }
    else if(strcmp(role,"rx") == 0) {
        str.role = LlRx;
    }

    llopen(str);
    resetAlarm();
    if(str.role == LlTx) {
        unsigned char pack[MAX_PAYLOAD_SIZE];
        unsigned char buf[MAX_PAYLOAD_SIZE];

        FILE* src = fopen(filename,"r");
        int filesz = get_file_size(src);

        int ctrlsize = mountCtrlPacket(&pack,0,filename,filesz);
        llwrite(&pack,ctrlsize);
        int bytes_r;
        int sq = 0;
        resetAlarm();
        while(bytes_r = fread(buf,1,MAX_PAYLOAD_SIZE - 4,src)) {
            unsigned char data[MAX_PAYLOAD_SIZE];
            int alo = mountDataPacket(&data,sq,bytes_r,buf);
            llwrite(&data,alo);
            resetAlarm();
            fprintf(stderr, "Data Packet Sent: Size: %d\n", alo);
            sq++;
        }

        fprintf(stderr, "Control Packet End Sent:\n");
        ctrlsize = mountCtrlPacket(&pack,1,filename,filesz);
        llwrite(&pack,ctrlsize);
        resetAlarm();
    }
    else if(str.role == LlRx) {
        int* filesz;
        char* filen;
        unsigned char pack[MAX_PAYLOAD_SIZE + 7];
        unsigned char data[MAX_PAYLOAD_SIZE + 7];

        int sz = llread(&pack);

        FILE* dest = fopen(filename,"w");
        int ns = 0;
        
        while(1) {
            int sq;
            int lastnr = nr;
            do{
                sz = llread(&pack);
                printf("reading package\n");
            }while (sz == -1);
            
        
            fprintf(stderr,"New Packet:\n");
            if(pack[0] == 0x03) {
                fprintf(stderr,"Read End\n");
                break;
            }
            else if(pack[0] == 0x01){
                sz = readDataPacket(&data,&pack,&sq);
                printf("\n\n");
                /*for(int i = 0; i < sz; i++) {
                    fprintf(stderr,"\\%02x", data[i]);
                }*/              
                fwrite(data,1,sz,dest);
            }
        }
    }
    llclose(0);
}
