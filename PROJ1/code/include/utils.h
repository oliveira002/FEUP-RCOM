#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// returns file size in bytes
int get_file_size(FILE* f);

// mounts control packet
int mountCtrlPacket(unsigned char* buf, int type, char* filename, int filesz);

// returns nr of bytes to represent a number
int numOfBytes(int filesz);

// mounts data packet
int mountDataPacket(unsigned char* buf, int sq, int sz, unsigned char* data);

// parses control packet
int readControlPacket(unsigned char* buf, char* filename, int* filesz);

// parses data packet
int readDataPacket(unsigned char* data, unsigned char* buf, int* seq);