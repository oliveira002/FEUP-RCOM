#include "state_machine.h"
#include <stdio.h>



void changeState(enum stateMachine* st, unsigned char byte, int type) {
    unsigned char store[2];
    if(type == 0) {
        switch (*st)
        {
        case START:
            if(byte == FLAG) {
                *st = FLAG_RCV;
            }
            else {
                *st = START;
            }
            break;
        
        case FLAG_RCV:
            if(byte == TRANS || byte == RECEIVE) { 
                store[0] = byte;
                *st = A_RCV;
            }
            else if(byte == FLAG) {
                *st = FLAG_RCV;
            }
            else {
                *st = START;
            }
            break;

        case A_RCV:
            if((byte == UA) || (byte == SET) || (byte == DISC) || (byte == 0x85) || (byte == 0x05)) { 
                store[1] = byte;
                *st = C_RCV;
            }else if( (byte == 0x01) || (byte == 0x81) ){
                printf("BAD WRITE\n");
                store[1] = byte;
                *st = REJ;
            }
            else if(byte == FLAG) {
                *st = FLAG_RCV;
            }
            else if((byte == ICTRL_OFF) || (byte == ICTRL_ON)) {
                store[1] = byte;
                *st = C_INF;
            }
            else {
                *st = START;
            }
            break;

        case C_RCV:
            if(byte == BCC(store[0],store[1])) { //se  A^C = BBC ?
                *st = BCC_OK;
            }
            else if(byte == FLAG) {
                *st = FLAG_RCV;
            }
            else {
                *st = START;
            }
            break;
        case REJ:
            break;
        case BCC_OK:
            if(byte == FLAG) {
                *st = STOP;
            }
            else {
                *st = START;
            }
            break;
        
        case STOP:
            break;

        default:
            break;
        }
    }
    else if(type == 1) { // information frame
        switch (*st)
        {
        case START:
            if(byte == FLAG) {
                *st = FLAG_RCV;
            }
            else {
                *st = START;
            }
            break;
        
        case FLAG_RCV:
            if(byte == TRANS || byte == RECEIVE) { 
                store[0] = byte;
                *st = A_RCV;
            }
            else if(byte == FLAG) {
                *st = FLAG_RCV;
            }
            else {
                *st = START;
            }
            break;

        case A_RCV:
            if(byte == FLAG) {
                *st = FLAG_RCV;
            }
            else if((byte == ICTRL_OFF) || (byte == ICTRL_ON)) {
                store[1] = byte;
                *st = C_INF;
            }
            else {
                *st = START;
            }
            break;

        case C_INF:
            if(byte == BCC(store[0],store[1])) { //se  A^C = BBC ?
                *st = BCC_OK;
            }
            else if(byte == FLAG) {
                *st = FLAG_RCV;
            }
            else {
                *st = START;
            }
            break;

        case BCC_OK:
            if(byte == FLAG) {
                *st = STOP;
            }
            break;
        
        case STOP:
            break;

        default:
            break;
        }
    }
}
