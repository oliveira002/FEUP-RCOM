#include "utils.h"

int get_file_size(FILE* f) {
    int s;
    fseek(f, 0, SEEK_END);
    s = (int) ftell(f);
    fseek(f, 0, SEEK_SET);
    return s;
}

int numOfBytes(int filesz) {
    int counter = 0;
    while(filesz > 0) {
        filesz /= 256;
        counter++;
    }
    return counter;
}

int mountCtrlPacket(unsigned char* buf, int type, char* filename, int filesz) {
    if(type == 0) {
        buf[0] = 0x02;
    }
    else if(type == 1) {
        buf[0] = 0x03;
    }

    unsigned L1 = numOfBytes(filesz);
    unsigned L2 = strlen(filename);
    unsigned ctrlsz = 5 + L1 + L2;

    buf[1] = 0x00;
    buf[2] = L1;

    for(int i = 0; i < L1; i++) {
        int tmp = (filesz & 0x0000FFFF) >> 8;
        buf[3 + i] = tmp;
        filesz = filesz << 8;
    }

    // memcpy(&buf[3],&filesz, L1);
    int nxt = L1 + 3;
    buf[nxt] = 0x01;
    buf[nxt + 1] = L2;
    memcpy(&buf[nxt + 2],filename, L2);

    /*for(int i = 4; i < 4 + ctrlsz; i++) {
        fprintf(stderr,"0x%02X, ", buf[i]);
    }*/

    return ctrlsz;
}

int mountDataPacket(unsigned char* buf, int sq, int sz, unsigned char* data) {
    buf[0] = 0x01;
    buf[1] = (sq % 255);
    buf[2] = (sz / 256);
    buf[3] = (sz % 256);

    memcpy(buf + 4,data,sz);
    /*for(int i = 4; i < 4 + sz; i++) {
        fprintf(stderr,"0x%02X, ", buf[i]);
    }*/
    printf("\n\n");
    return (4 + sz);
}

int readControlPacket(unsigned char* buf, char* filename, int* filesz) {
    *filesz = 0;
    int fst;
    if(buf[0] == 0x02 && buf[0] == 0x03) {
        fprintf(stderr,"not ctrl packet");
        return -1;
    }
    
    if(buf[1] == 0x00) {
        fst = buf[2];
        for(int i = 0; i < fst; i++) {
            *filesz = *filesz * 256 + buf[3+i];
        }
    }
    else {
        return -1;
    }
    int snd = 0;
    int nxt = 5 + fst;

    if(buf[nxt - 2] == 0x01) {
        snd = buf[nxt - 1];
        for(int j = 0; j < snd; j++) {
            filename[j] = buf[j+nxt];
        }
    }
    else {
        return -1;
    }
    return 0;
}

int readDataPacket(unsigned char* data, unsigned char* buf, int* seq) {
    if(buf[0] != 0x01) {
        fprintf(stderr,"Not Data");
        return -1;
    }
    *seq = buf[1];
    int l2 = buf[2];
    int l1 = buf[3];
    int sz = (256 * l2) + l1;

    for(int i = 0; i < sz; i++) {
        data[i] = buf[4+i];
    } 
    return sz;
}