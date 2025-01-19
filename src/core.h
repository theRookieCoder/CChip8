#ifndef _OTHER_H_
#define _OTHER_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct MachineState {
    uint8_t ram[4096];

    bool dispBuf[64][32];

    uint16_t programCounter;
    uint16_t indexReg;

    uint8_t varRegs[16];

    uint16_t stack[16];
    uint8_t stackIdx;

    uint8_t delayTimer;
    uint8_t soundTimer;
} MachineState;

/**
 * Initialise the core's machine state.
 *
 * The machine state is initialised in `static` memory, and the font is loaded
 * into address `0x050`.
 *
 * @return Pointer to the initialised machine state
 */
MachineState* core_init();

/**
 * Load a program from `romFile` into `self`'s RAM.
 *
 * Only `0x0E00` bytes from the `romFile` are loaded, since that is the maximum
 * amount the CHIP-8's RAM can hold.
 *
 * @param self      The machine state to load the ROM into
 * @param romFile   The ROM file to load
 */
void core_loadROM(MachineState* self, FILE* romFile);

/**
 * Tick the machine state's delay and sound timers.
 *
 * This should be done 60 times per second (at 60 Hz).
 *
 * @param self  The machine state to tick
 */
void core_timerTick(MachineState* self);

/**
 * Execute a single instruction.
 *
 * @param self      The machine state
 * @param heldKeys  A pointer to a function that returns 16 bits corresponding
 *                  to the 16 keys
 *
 * @return          Whether the machine state's display buffer was updated
 */
bool core_tick(MachineState* self, uint16_t (*heldKeys)());

#endif
