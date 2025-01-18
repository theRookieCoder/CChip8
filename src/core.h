#ifndef _OTHER_H_
#define _OTHER_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct MachineState {
    uint8_t ram[4096];

    bool dispBuf[64][32];

    uint16_t stack[16];
    uint8_t stackIdx;

    uint8_t delayTimer;
    uint8_t soundTimer;

    uint16_t programCounter;
    uint16_t indexReg;

    uint8_t varRegs[16];
} MachineState;

MachineState* core_init();
void core_loadROM(MachineState* self, FILE* romFile);
void core_timerTick(MachineState* self);
bool core_tick(MachineState* self, uint16_t (*heldKeys)());

#endif
