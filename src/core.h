#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Holds the state of the emulated machine
typedef struct MachineState {
#ifndef CORE_RAM_SIZE
#define CORE_RAM_SIZE 4096
#endif
    /// The emulated RAM.
    /// Its size can be controlled using the `CORE_RAM_SIZE` macro, which
    /// defaults to `4096`.
    uint8_t ram[CORE_RAM_SIZE];

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
     * Tests for key input.
     *
     * @returns Bitflags of the keys that are held
     */
    uint16_t (*heldKeys)();

    /**
     * Gets the state of the pixel at the coordinates (x, y).
     *
     * Should automatically wrap if the coordinates exceed the display size of
     * 64x32.
     *
     * @param x x coordinate of the pixel to query
     * @param y y coordinate of the pixel to query
     *
     * @returns Whether the pixel is on or off
     */
    bool (*getPixel)(uint8_t x, uint8_t y);

    /**
     * Toggles the state of the pixel at the coordinates (x, y).
     *
     * Should automatically wrap if the coordinates exceed the display size of
     * 64x32.
     *
     * @param x x coordinate of the pixel to toggle
     * @param y y coordinate of the pixel to toggle
     */
    void (*togglePixel)(uint8_t x, uint8_t y);

    /// Clears the display to off
    void (*clearDisplay)();

    /// Handles an illegal instruction
    void (*sigIllHandler)();
} MachineState;

/**
 * Initialises the machine state at `p_machineState`.
 *
 * @param p_machineState    The machine state to initialise
 * @param p_font            The font to load, uses a default font if NULL
 * @param fontCopy          The function to use to copy the font, uses `memcpy`
 *                          if NULL
 *
 * @see `MachineState` for documentation about the callbacks
 */
void core_init(MachineState* p_machineState,
               const uint8_t p_font[16 * 5],
               void*(fontCopy)(void* dest, const void* src, size_t count),
               uint16_t (*heldKeys)(),
               bool (*getPixel)(uint8_t x, uint8_t y),
               void (*togglePixel)(uint8_t x, uint8_t y),
               void (*clearDisplay)(),
               void (*sigIllHandler)());

/**
 * Ticks `p_machineState`'s delay and sound timers.
 *
 * This should be called 60 times per second (at 60 Hz).
 *
 * @param p_machineState    The machine state to tick
 */
void core_timerTick(MachineState* p_machineState);

/**
 * Executes a single instruction, and updates `p_machineState` accordingly.
 *
 * @param p_machineState    The machine state to use
 *
 * @return Whether the display buffer was updated
 */
bool core_tick(MachineState* p_machineState);
