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

void push(MachineState* self, uint16_t val) {
    if (self->stackIdx > 16) printf("Stack overflow!\n");

    self->stack[(self->stackIdx)++] = val;
}

uint16_t pop(MachineState* self) { return self->stack[--(self->stackIdx)]; }

MachineState* core_init() {
    static MachineState self = {.programCounter = 0x200};

    memcpy(&self.ram[0x50], FONT, sizeof(FONT));

    return &self;
}

void core_loadROM(MachineState* self, FILE* romFile) {
    int i = 0;
    int written = 0;
    while ((written = fread(
                &self->ram[0x200 + i], sizeof *(self->ram), 1, romFile)) > 0)
        i += written;
}

#define V0 self->varRegs[0x0]
#define V1 self->varRegs[0x1]
#define V2 self->varRegs[0x2]
#define V3 self->varRegs[0x3]
#define V4 self->varRegs[0x4]
#define V5 self->varRegs[0x5]
#define V6 self->varRegs[0x6]
#define V7 self->varRegs[0x7]
#define V8 self->varRegs[0x8]
#define V9 self->varRegs[0x9]
#define VA self->varRegs[0xA]
#define VB self->varRegs[0xB]
#define VC self->varRegs[0xC]
#define VD self->varRegs[0xD]
#define VE self->varRegs[0xE]
#define VF self->varRegs[0xF]
#define VX self->varRegs[X]
#define VY self->varRegs[Y]

void core_timerTick(MachineState* self) {
    if (self->delayTimer > 0) self->delayTimer--;
    if (self->soundTimer > 0) self->soundTimer--;
}

bool core_tick(MachineState* self, uint16_t (*heldKeys)()) {
    /* FETCH */
    uint16_t instruction = (self->ram[self->programCounter] << 8) +
                           self->ram[self->programCounter + 1];
    self->programCounter += 2;
#if DEBUG
    printf("\nInstruction: %04X\n", instruction);
#endif


    /* DECODE */
#define X ((instruction & 0x0F00) >> 8)
#define Y ((instruction & 0x00F0) >> 4)
#define N (instruction & 0x000F)
#define NN (instruction & 0x00FF)
#define NNN (instruction & 0x0FFF)

// Display machine state
#if DEBUG
    printf("PC: 0x%04X\n", self->programCounter - 2);
    printf("Stack:\n");
    for (int i = 0; i < 16; i++) printf("    0x%04X,\n", self->stack[i]);
    printf("SP: %i\n", self->stackIdx);
    printf("I : 0x%04X\n", self->indexReg);
    printf("V : ");
    for (int i = 0; i < 16; i++) printf("0x%02X, ", self->varRegs[i]);
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
                            memset(self->dispBuf, 0, sizeof(self->dispBuf));
                            updateDisp = true;
                            break;

                        case 0xE:
                            self->programCounter = pop(self);
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
            self->programCounter = NNN;
            break;

        case 0x2:
            push(self, self->programCounter);
            self->programCounter = NNN;
            break;

        case 0x3:
            if (VX == NN) self->programCounter += 2;
            break;

        case 0x4:
            if (VX != NN) self->programCounter += 2;
            break;

        case 0x5:
            if (VX == VY) self->programCounter += 2;
            break;

        case 0x9:
            if (VX != VY) self->programCounter += 2;
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
            self->indexReg = NNN;
            break;

        case 0xB:
            self->programCounter = NNN + V0;
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
                uint8_t nthSpriteRow = self->ram[self->indexReg + i];
                for (int j = 7; j >= 0; j--) {
                    bool bit = (nthSpriteRow & (1 << j)) >> j;
                    if (bit) {
                        VF |= self->dispBuf[x][y];
                        self->dispBuf[x][y] = !self->dispBuf[x][y];
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
                    if ((heldKeys() >> VX) & 0b1) self->programCounter += 2;
                    break;

                case 0xA1:
                    if (!((heldKeys() >> VX) & 0b1)) self->programCounter += 2;
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
                    VX = self->delayTimer;
                    break;

                case 0x15:
                    self->delayTimer = VX;
                    break;

                case 0x18:
                    self->soundTimer = VX;
                    break;

                case 0x1E:
                    self->indexReg += VX;
                    if (self->indexReg > 0x1000)
                        VF = 1;  // Amiga specific behaviour
                    break;

                case 0x0A:
                    if (heldKeys() == 0)
                        self->programCounter -= 2;
                    else
                        for (int i = 0; i < 16; i++)
                            if (heldKeys() >> i & 0b1) VX = i;

                    break;

                case 0x29:
                    self->indexReg = 0x50 + (VX & 0xF) * 5;
                    break;

                case 0x33:
                    self->ram[self->indexReg + 2] = (VX / 1) % 10;
                    self->ram[self->indexReg + 1] = (VX / 10) % 10;
                    self->ram[self->indexReg + 0] = (VX / 100) % 10;
                    break;

                case 0x55:
                    for (int i = 0; i <= X; i++)
                        self->ram[self->indexReg + i] = self->varRegs[i];
                    break;

                case 0x65:
                    for (int i = 0; i <= X; i++)
                        self->varRegs[i] = self->ram[self->indexReg + i];
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
