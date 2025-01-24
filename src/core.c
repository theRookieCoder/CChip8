#include "core.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Squeeze the font into the space just before the program
#define FONT_ADDR 0x0200 - 16 * 5
const uint8_t DEFAULT_FONT[16 * 5] = {
    // 0
    0b11110000,
    0b10010000,
    0b10010000,
    0b10010000,
    0b11110000,

    // 1
    0b00100000,
    0b01100000,
    0b00100000,
    0b00100000,
    0b01110000,

    // 2
    0b11110000,
    0b00010000,
    0b11110000,
    0b10000000,
    0b11110000,

    // 3
    0b11110000,
    0b00010000,
    0b11110000,
    0b00010000,
    0b11110000,

    // 4
    0b10010000,
    0b10010000,
    0b11110000,
    0b00010000,
    0b00010000,

    // 5
    0b11110000,
    0b10000000,
    0b11110000,
    0b00010000,
    0b11110000,

    // 6
    0b11110000,
    0b10000000,
    0b11110000,
    0b10010000,
    0b11110000,

    // 7
    0b11110000,
    0b00010000,
    0b00100000,
    0b01000000,
    0b01000000,

    // 8
    0b11110000,
    0b10010000,
    0b11110000,
    0b10010000,
    0b11110000,

    // 9
    0b11110000,
    0b10010000,
    0b11110000,
    0b00010000,
    0b11110000,

    // A
    0b11110000,
    0b10010000,
    0b11110000,
    0b10010000,
    0b10010000,

    // B
    0b11100000,
    0b10010000,
    0b11100000,
    0b10010000,
    0b11100000,

    // C
    0b11110000,
    0b10000000,
    0b10000000,
    0b10000000,
    0b11110000,

    // D
    0b11100000,
    0b10010000,
    0b10010000,
    0b10010000,
    0b11100000,

    // E
    0b11110000,
    0b10000000,
    0b11110000,
    0b10000000,
    0b11110000,

    // F
    0b11110000,
    0b10000000,
    0b11110000,
    0b10000000,
    0b10000000,
};


void push(MachineState* p_machineState, uint16_t val) {
    if (p_machineState->stackIdx > 16) printf("Stack overflow!\n");

    p_machineState->stack[(p_machineState->stackIdx)++ % 16] = val;
}

uint16_t pop(MachineState* p_machineState) {
    return p_machineState->stack[--(p_machineState->stackIdx)];
}


void core_init(MachineState* p_machineState,
               const uint8_t p_font[16 * 5],
               uint16_t (*heldKeys)(),
               bool (*getPixel)(uint8_t x, uint8_t y),
               void (*togglePixel)(uint8_t x, uint8_t y),
               void (*clearDisplay)()) {
    p_machineState->programCounter = 0x0200;
    p_machineState->heldKeys = heldKeys;
    p_machineState->getPixel = getPixel;
    p_machineState->togglePixel = togglePixel;
    p_machineState->clearDisplay = clearDisplay;

    memcpy(&p_machineState->ram[FONT_ADDR],
           (p_font != NULL) ? p_font : DEFAULT_FONT,
           16 * 5);
}

#define V0 p_machineState->varRegs[0x0]
#define V1 p_machineState->varRegs[0x1]
#define V2 p_machineState->varRegs[0x2]
#define V3 p_machineState->varRegs[0x3]
#define V4 p_machineState->varRegs[0x4]
#define V5 p_machineState->varRegs[0x5]
#define V6 p_machineState->varRegs[0x6]
#define V7 p_machineState->varRegs[0x7]
#define V8 p_machineState->varRegs[0x8]
#define V9 p_machineState->varRegs[0x9]
#define VA p_machineState->varRegs[0xA]
#define VB p_machineState->varRegs[0xB]
#define VC p_machineState->varRegs[0xC]
#define VD p_machineState->varRegs[0xD]
#define VE p_machineState->varRegs[0xE]
#define VF p_machineState->varRegs[0xF]
#define VX p_machineState->varRegs[X]
#define VY p_machineState->varRegs[Y]

void core_timerTick(MachineState* p_machineState) {
    if (p_machineState->delayTimer > 0) p_machineState->delayTimer--;
    if (p_machineState->soundTimer > 0) p_machineState->soundTimer--;
}

bool core_tick(MachineState* p_machineState) {
    /* FETCH */
    uint16_t instruction =
        (p_machineState->ram[p_machineState->programCounter % CORE_RAM_SIZE]
         << 8) +
        p_machineState
            ->ram[(p_machineState->programCounter + 1) % CORE_RAM_SIZE];
    p_machineState->programCounter += 2;


    /* DECODE */
#define X ((instruction & 0x0F00) >> 8)
#define Y ((instruction & 0x00F0) >> 4)
#define N (instruction & 0x000F)
#define NN (instruction & 0x00FF)
#define NNN (instruction & 0x0FFF)

// Display machine state
#if DEBUG
    printf("\nPC: 0x%04X\n", p_machineState->programCounter - 2);
    printf("Instruction: %04X\n", instruction);
    printf("Stack: ");
    for (int i = 0; i < p_machineState->stackIdx; i++)
        printf("%03X, ", p_machineState->stack[i]);
    printf("\n");
    printf("SP: %i\n", p_machineState->stackIdx);
    printf("I : 0x%04X\n", p_machineState->indexReg);
    printf("V : ");
    for (int i = 0; i < 16; i++) printf("0x%02X, ", p_machineState->varRegs[i]);
    printf("\n");
    printf("VX: 0x%02X\n", VX);
    printf("VY: 0x%02X\n", VY);
#endif


    /* EXECUTE */
    bool updateDisp = false;

    switch ((instruction & 0xF000) >> 12) {
        case 0x0: {
            switch (Y) {
                case 0xE: {
                    switch (N) {
                        case 0x0:
                            p_machineState->clearDisplay();
                            updateDisp = true;
                            break;

                        case 0xE:
                            p_machineState->programCounter =
                                pop(p_machineState);
                            break;
#if DEBUG
                        default:
                            printf("Instruction not implemented\n");
#endif
                    }

                    break;
                }
#if DEBUG
                default:
                    printf("Instruction not implemented\n");
#endif
            }

            break;
        }

        case 0x1:
            p_machineState->programCounter = NNN;
            break;

        case 0x2:
            push(p_machineState, p_machineState->programCounter);
            p_machineState->programCounter = NNN;
            break;

        case 0x3:
            if (VX == NN) p_machineState->programCounter += 2;
            break;

        case 0x4:
            if (VX != NN) p_machineState->programCounter += 2;
            break;

        case 0x5:
            if (VX == VY) p_machineState->programCounter += 2;
            break;

        case 0x9:
            if (VX != VY) p_machineState->programCounter += 2;
            break;

        case 0x8: {
            switch (N) {
                case 0x0:
                    VX = VY;
                    break;

                case 0x1:
                    VX |= VY;
                    VF = 0;
                    break;

                case 0x2:
                    VX &= VY;
                    VF = 0;
                    break;

                case 0x3:
                    VX ^= VY;
                    VF = 0;
                    break;

                case 0x4: {
                    uint64_t vx = VX;
                    uint64_t vy = VY;
                    VX = vx + vy;
                    VF = (vx + vy > 0xFF) ? 1 : 0;
                    break;
                }

                case 0x5: {
                    uint64_t vx = VX;
                    uint64_t vy = VY;
                    VX = vx - vy;
                    VF = (vx >= vy) ? 1 : 0;
                    break;
                }

                case 0x7: {
                    uint64_t vx = VX;
                    uint64_t vy = VY;
                    VX = vy - vx;
                    VF = (vy >= vx) ? 1 : 0;
                    break;
                }

                case 0x6: {
                    bool shiftedOut = VY & 0b00000001;
                    VX = VY >> 1;
                    VF = shiftedOut;
                    break;
                }

                case 0xE: {
                    bool shiftedOut = (VY & 0b10000000) >> 7;
                    VX = VY << 1;
                    VF = shiftedOut;
                    break;
                }
#if DEBUG
                default:
                    printf("Instruction not implemented\n");
#endif
            }

            break;
        }

        case 0x6:
            VX = NN;
            break;

        case 0x7:
            VX += NN;
            break;

        case 0xA:
            p_machineState->indexReg = NNN;
            break;

        case 0xB:
            p_machineState->programCounter = NNN + V0;
            break;

        case 0xC:
            VX = rand() & NN;
            break;

        case 0xD: {
            updateDisp = true;
            int start_x = VX % 64;
            int y = VY % 32;
            VF = 0;

            for (int i = 0; i < N; i++) {
                int x = start_x;
                uint8_t nthSpriteRow =
                    p_machineState
                        ->ram[(p_machineState->indexReg + i) % CORE_RAM_SIZE];
                for (int j = 7; j >= 0; j--) {
                    bool bit = (nthSpriteRow & (1 << j)) >> j;
                    if (bit) {
                        VF |= p_machineState->getPixel(x, y);
                        p_machineState->togglePixel(x, y);
                    }
                    x++;
                    if (x > 63) break;
                }
                y++;
                if (y > 31) break;
            }

            break;
        }

        case 0xE: {
            switch (NN) {
                case 0x9E:
                    if ((p_machineState->heldKeys() >> VX) & 0b1)
                        p_machineState->programCounter += 2;
                    break;

                case 0xA1:
                    if (!((p_machineState->heldKeys() >> VX) & 0b1))
                        p_machineState->programCounter += 2;
                    break;
#if DEBUG
                default:
                    printf("Instruction not implemented\n");
#endif
            }

            break;
        }

        case 0xF: {
            switch (NN) {
                case 0x07:
                    VX = p_machineState->delayTimer;
                    break;

                case 0x15:
                    p_machineState->delayTimer = VX;
                    break;

                case 0x18:
                    p_machineState->soundTimer = VX;
                    break;

                case 0x1E:
                    p_machineState->indexReg += VX;
                    if (p_machineState->indexReg > 0x1000)
                        VF = 1;  // Amiga specific behaviour
                    break;

                case 0x0A:
                    if (p_machineState->heldKeys() == 0)
                        p_machineState->programCounter -= 2;
                    else
                        for (int i = 0; i < 16; i++)
                            if (p_machineState->heldKeys() >> i & 0b1) VX = i;

                    break;

                case 0x29:
                    p_machineState->indexReg = FONT_ADDR + (VX & 0xF) * 5;
                    break;

                case 0x33:
                    p_machineState
                        ->ram[(p_machineState->indexReg + 2) % CORE_RAM_SIZE] =
                        (VX / 1) % 10;
                    p_machineState
                        ->ram[(p_machineState->indexReg + 1) % CORE_RAM_SIZE] =
                        (VX / 10) % 10;
                    p_machineState
                        ->ram[(p_machineState->indexReg + 0) % CORE_RAM_SIZE] =
                        (VX / 100) % 10;
                    break;

                case 0x55:
                    for (int i = 0; i <= X; i++)
                        p_machineState
                            ->ram[p_machineState->indexReg++ % CORE_RAM_SIZE] =
                            p_machineState->varRegs[i];
                    break;

                case 0x65:
                    for (int i = 0; i <= X; i++)
                        p_machineState->varRegs[i] =
                            p_machineState->ram[p_machineState->indexReg++ %
                                                CORE_RAM_SIZE];
                    break;
#if DEBUG
                default:
                    printf("Instruction not implemented\n");
#endif
            }

            break;
        }

#if DEBUG
        default:
            printf("Instruction not implemented\n");
#endif
    }

    return updateDisp;
}
