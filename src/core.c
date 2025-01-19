#include "core.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>  // rand()
#include <string.h>  // memcpy() and memset()

const uint8_t FONT[16 * 5] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

void push(MachineState* p_self, uint16_t val) {
    if (p_self->stackIdx > 16) printf("Stack overflow!\n");

    p_self->stack[(p_self->stackIdx)++ % 16] = val;
}

uint16_t pop(MachineState* p_self) {
    return p_self->stack[--(p_self->stackIdx)];
}

MachineState* core_init() {
    static MachineState p_self = {.programCounter = 0x200};

    memcpy(&p_self.ram[0x50], FONT, sizeof(FONT));

    return &p_self;
}

void core_loadROM(MachineState* p_self, FILE* romFile) {
    int read = fread(
        &p_self->ram[0x200], sizeof(*(p_self->ram)), 0x1000 - 0x0200, romFile);
#if DEBUG
    printf("Loaded %i bytes into machine state's RAM.\n", read);
#endif
}

#define V0 p_self->varRegs[0x0]
#define V1 p_self->varRegs[0x1]
#define V2 p_self->varRegs[0x2]
#define V3 p_self->varRegs[0x3]
#define V4 p_self->varRegs[0x4]
#define V5 p_self->varRegs[0x5]
#define V6 p_self->varRegs[0x6]
#define V7 p_self->varRegs[0x7]
#define V8 p_self->varRegs[0x8]
#define V9 p_self->varRegs[0x9]
#define VA p_self->varRegs[0xA]
#define VB p_self->varRegs[0xB]
#define VC p_self->varRegs[0xC]
#define VD p_self->varRegs[0xD]
#define VE p_self->varRegs[0xE]
#define VF p_self->varRegs[0xF]
#define VX p_self->varRegs[X]
#define VY p_self->varRegs[Y]

void core_timerTick(MachineState* p_self) {
    if (p_self->delayTimer > 0) p_self->delayTimer--;
    if (p_self->soundTimer > 0) p_self->soundTimer--;
}

bool core_tick(MachineState* p_self, uint16_t (*heldKeys)()) {
    /* FETCH */
    uint16_t instruction = (p_self->ram[p_self->programCounter] << 8) +
                           p_self->ram[p_self->programCounter + 1];
    p_self->programCounter += 2;


    /* DECODE */
#define X ((instruction & 0x0F00) >> 8)
#define Y ((instruction & 0x00F0) >> 4)
#define N (instruction & 0x000F)
#define NN (instruction & 0x00FF)
#define NNN (instruction & 0x0FFF)

// Display machine state
#if DEBUG
    printf("\nPC: 0x%04X\n", p_self->programCounter - 2);
    printf("Instruction: %04X\n", instruction);
    printf("Stack: ");
    for (int i = 0; i < p_self->stackIdx; i++)
        printf("%03X, ", p_self->stack[i]);
    printf("\n");
    printf("SP: %i\n", p_self->stackIdx);
    printf("I : 0x%04X\n", p_self->indexReg);
    printf("V : ");
    for (int i = 0; i < 16; i++) printf("0x%02X, ", p_self->varRegs[i]);
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
                            memset(p_self->dispBuf, 0, sizeof(p_self->dispBuf));
                            updateDisp = true;
                            break;

                        case 0xE:
                            p_self->programCounter = pop(p_self);
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
            p_self->programCounter = NNN;
            break;

        case 0x2:
            push(p_self, p_self->programCounter);
            p_self->programCounter = NNN;
            break;

        case 0x3:
            if (VX == NN) p_self->programCounter += 2;
            break;

        case 0x4:
            if (VX != NN) p_self->programCounter += 2;
            break;

        case 0x5:
            if (VX == VY) p_self->programCounter += 2;
            break;

        case 0x9:
            if (VX != VY) p_self->programCounter += 2;
            break;

        case 0x8: {
            switch (N) {
                case 0x0:
                    VX = VY;
                    break;

                case 0x1:
                    VX |= VY;
                    break;

                case 0x2:
                    VX &= VY;
                    break;

                case 0x3:
                    VX ^= VY;
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
                    bool shiftedOut = VX & 0b00000001;
                    VX = VX >> 1;
                    VF = shiftedOut;
                    break;
                }

                case 0xE: {
                    bool shiftedOut = (VX & 0b10000000) >> 7;
                    VX = VX << 1;
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
            p_self->indexReg = NNN;
            break;

        case 0xB:
            p_self->programCounter = NNN + V0;
            break;

        case 0xC:
            VX = rand() & NN;
            break;

        case 0xD: {
            updateDisp = true;
            int x = VX % 64;
            int y = VY % 32;
            VF = 0;

            for (int i = 0; i < N; i++) {
                x = VX % 64;
                uint8_t nthSpriteRow = p_self->ram[p_self->indexReg + i];
                for (int j = 7; j >= 0; j--) {
                    bool bit = (nthSpriteRow & (1 << j)) >> j;
                    if (bit) {
                        VF |= p_self->dispBuf[x][y];
                        p_self->dispBuf[x][y] = !p_self->dispBuf[x][y];
                    }
                    x++;
                    if (x > 63) break;
                }
                y++;
            }

            break;
        }

        case 0xE: {
            switch (NN) {
                case 0x9E:
                    if ((heldKeys() >> VX) & 0b1) p_self->programCounter += 2;
                    break;

                case 0xA1:
                    if (!((heldKeys() >> VX) & 0b1))
                        p_self->programCounter += 2;
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
                    VX = p_self->delayTimer;
                    break;

                case 0x15:
                    p_self->delayTimer = VX;
                    break;

                case 0x18:
                    p_self->soundTimer = VX;
                    break;

                case 0x1E:
                    p_self->indexReg += VX;
                    if (p_self->indexReg > 0x1000)
                        VF = 1;  // Amiga specific behaviour
                    break;

                case 0x0A:
                    if (heldKeys() == 0)
                        p_self->programCounter -= 2;
                    else
                        for (int i = 0; i < 16; i++)
                            if (heldKeys() >> i & 0b1) VX = i;

                    break;

                case 0x29:
                    p_self->indexReg = 0x50 + (VX & 0xF) * 5;
                    break;

                case 0x33:
                    p_self->ram[p_self->indexReg + 2] = (VX / 1) % 10;
                    p_self->ram[p_self->indexReg + 1] = (VX / 10) % 10;
                    p_self->ram[p_self->indexReg + 0] = (VX / 100) % 10;
                    break;

                case 0x55:
                    for (int i = 0; i <= X; i++)
                        p_self->ram[p_self->indexReg + i] = p_self->varRegs[i];
                    break;

                case 0x65:
                    for (int i = 0; i <= X; i++)
                        p_self->varRegs[i] = p_self->ram[p_self->indexReg + i];
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
