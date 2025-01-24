#ifndef _OTHER_H_
#define _OTHER_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/**
 * Holds the state of the emulated machine
 */
typedef struct MachineState {
    /// The emulated RAM.
    /// Its size can be controlled using the `CORE_RAM_SIZE` macro, which
    /// defaults to `4096`.
#ifndef CORE_RAM_SIZE
#define CORE_RAM_SIZE 4096
#endif
    uint8_t ram[CORE_RAM_SIZE];

    /// Address to load the next instruction from
    uint16_t programCounter;
    uint16_t indexReg;
    uint8_t varRegs[16];

    /// Stores return addresses when jumping to subroutines
    uint16_t stack[16];
    /// The number of addresses stored on the stack
    uint8_t stackIdx;

    uint8_t delayTimer;
    uint8_t soundTimer;

    /* CALLBACKS */

    /**
     * Called when key input needs to be tested
     *
     * @returns Bitflags of the keys that are held
     */
    uint16_t (*heldKeys)();

    /**
     * Gets the state of the pixel at the coordinates (x, y).
     *
     * Automatically wraps if the coordinates exceed the display size.
     *
     * @param x x coordinate of the pixel to query
     * @param y y coordinate of the pixel to query
     *
     * @returns Whether the pixel is on or off
     */
    bool (*getPixel)(uint8_t x, uint8_t y);

    /**
     * Toggles the state of the pixel at the coordinates (x, y)
     *
     * Automatically wraps if the coordinates exceed the display size.
     *
     * @param x x coordinate of the pixel to toggle
     * @param y y coordinate of the pixel to toggle
     */
    void (*togglePixel)(uint8_t x, uint8_t y);

    /// Clears the display
    void (*clearDisplay)();
} MachineState;

/**
 * Initialise the core's machine state.
 *
 * The machine state is initialised in `static` memory.
 * The font is loaded into RAM at `0x0050` from `p_font`. A default font is
 * loaded if `p_font` is `NULL`.
 *
 * @param p_font        The font to load
 *
 * @returns Pointer to the initialised machine state
 *
 * @see `MachineState` for documentation about the callbacks
 */
MachineState* core_init(const uint8_t p_font[16 * 5],
                        uint16_t (*heldKeys)(),
                        bool (*getPixel)(uint8_t x, uint8_t y),
                        void (*togglePixel)(uint8_t x, uint8_t y),
                        void (*clearDisplay)());

/**
 * Load a program from `romFile` into `p_machineState`'s RAM.
 *
 * Only the amount of bytes the emulated RAM can hold, i.e.
 * `sizeof(p_machineState->ram) - 0x0200`, is loaded.
 *
 * @param p_machineState    The machine state to load the ROM into
 * @param romFile           The ROM file to load
 */
void core_loadROMfile(MachineState* p_machineState, FILE* romFile);

/**
 * Tick `p_machineState`'s delay and sound timers.
 *
 * This should be called 60 times per second (at 60 Hz).
 *
 * @param p_machineState  The machine state to tick
 */
void core_timerTick(MachineState* p_machineState);

/**
 * Execute a single instruction, and update `p_machineState` accordingly.
 *
 * @param p_machineState    The machine state
 *
 * @return Whether the display buffer was updated
 */
bool core_tick(MachineState* p_machineState);

#endif
