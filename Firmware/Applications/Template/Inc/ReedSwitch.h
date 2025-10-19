#include <stdint.h>

#pragma once 

typedef enum {
    kOpen,kClose
}ReadState;

void ReedSwitchInit(void);

ReadState ReedSwitchGetState(void);

uint8_t ChangeState(void);



