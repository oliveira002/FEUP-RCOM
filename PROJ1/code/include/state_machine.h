#include "macros.h"

enum stateMachine {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, C_INF, STOP, REJ};

// changes the state depending on its current state and the byte it receives
void changeState(enum stateMachine* st, unsigned char byte, int type); 
