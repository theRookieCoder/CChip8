#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define VERSION "0.0.1"
#define PROG_NAME "cchip8"

const uint8_t font[16 * 5] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

uint16_t stack[16] = {};
uint8_t stackPtr = 0;

uint8_t ram[4096] = {};

bool dispBuf[64][32] = {};

// uint8_t delayTimer = 0;
// uint8_t soundTimer = 0;

uint16_t programCounter = 0x200;
uint16_t indexReg = 0;

uint8_t varRegs[16] = {};
#define V0 varRegs[0x0]
#define V1 varRegs[0x1]
#define V2 varRegs[0x2]
#define V3 varRegs[0x3]
#define V4 varRegs[0x4]
#define V5 varRegs[0x5]
#define V6 varRegs[0x6]
#define V7 varRegs[0x7]
#define V8 varRegs[0x8]
#define V9 varRegs[0x9]
#define VA varRegs[0xA]
#define VB varRegs[0xB]
#define VC varRegs[0xC]
#define VD varRegs[0xD]
#define VE varRegs[0xE]
#define VF varRegs[0xF]
#define VX varRegs[X]
#define VY varRegs[Y]

void push(uint16_t val) {
    if (stackPtr > 16)
        printf("Stack overflow!\n");

    stack[stackPtr++] = val;
}

uint16_t pop() {
    return stack[--stackPtr];
}

int main(int argc, char **argv) {
    printf("%s version %s\n\n", PROG_NAME, VERSION);


    // Load font
    for (int i = 0; i < 16 * 5; i++) {
        ram[0x50 + i] = font[i];
    }

    if (argc != 2) {
        printf("Usage: cchip8 rom_file\n");
        return 1;
    }


    // Load program ROM
    FILE *rom_file = fopen(argv[1], "rb");

    if (rom_file == NULL) {
        printf("ROM file could not be opened\n");
        return 1;
    }

    int i = 0;
    int written = 0;
    while ((written = fread(&ram[0x200 + i], sizeof *ram, 1, rom_file)) > 0)
        i += written;

    fclose(rom_file);


    #if DEBUG
        // Dump RAM to the console
        printf("ADDR: DATA");
        for (int i = 0; i < 0x1000; i++) {
            if (i % 16 == 0) printf("\n%04X: ", i);
            printf("0x%02X ", ram[i]);
        }
        printf("\n");
    #endif


    uint16_t instruction = 0;

    while (true) {
        // Fetch
        instruction = (ram[programCounter] << 8) + ram[programCounter + 1];
        programCounter += 2;
        #if DEBUG
            printf("\nInstruction: %04X\n\n", instruction);
        #endif


        // Decode
        #define X   ((instruction & 0x0F00) >> 8)
        #define Y   ((instruction & 0x00F0) >> 4)
        #define N   (instruction & 0x000F)
        #define NN  (instruction & 0x00FF)
        #define NNN (instruction & 0x0FFF)

        // #if DEBUG
        //     printf("Decode:\n");
        //     printf("X   :   %01X\n", X);
        //     printf("Y   :   %01X\n", Y);
        //     printf("N   :   %01X\n", N);
        //     printf("NN  :  %02X\n", NN);
        //     printf("NNN : %03X\n\n", NNN);
        // #endif


        // Display machine state
        #if DEBUG
            printf("PC: 0x%04X\n", programCounter);
            printf("I : 0x%04X\n", indexReg);
            printf("V : ");
            for (int i = 0; i < 16; i++) {
                printf("0x%02X, ", varRegs[i]);
            }
            printf("\n");
            printf("VX: 0x%02X\n", VX);
            printf("VY: 0x%02X\n", VY);
        #endif


        // Execute
        switch ((instruction & 0xF000) >> 12) {
            case 0x0:
                switch (Y) {
                    case 0xE: {
                        switch (N) {
                            case 0x0:
                                memset(dispBuf, 0, sizeof(dispBuf));
                                break;

                            case 0xE:
                                programCounter = pop();
                        }
                    }

                    #if DEBUG
                    default:
                        printf("Y = %01X: Display instruction not implemented\n", Y);
                    #endif
                }
                break;

            case 0x1:
                programCounter = NNN;
                break;

            case 0x2:
                programCounter = NNN;
                push(programCounter);
                break;

            case 0x3:
                if (VX == NN) programCounter += 2;
                break;

            case 0x4:
                if (VX != NN) programCounter += 2;
                break;

            case 0x5:
                if (VX == VY) programCounter += 2;
                break;

            case 0x9:
                if (VX != VY) programCounter += 2;
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

                    case 0x4:
                        if (VX == 255)
                            VF = 1;
                        else
                            VF = 0;

                        VX += VY;
                        break;

                    case 0x5:
                        if (VX > VY)
                            VF = 1;
                        else
                            VF = 0;

                        VX = VX - VY;
                        break;

                    case 0x7:
                        if (VY > VX)
                            VF = 1;
                        else
                            VF = 0;

                        VX = VY - VX;
                        break;

                    case 0x6:
                        #if !SCHIP
                            VX = VY;
                        #endif
                        VF = VX & 0b1;
                        VX = VX >> 1;
                        break;
                }
            }

            case 0x6:
                VX = NN;
                break;

            case 0x7:
                VX += NN;
                break;

            case 0xA:
                indexReg = NNN;
                break;
            
            case 0xB:
                programCounter = NNN + V0;
                break;
            
            case 0xC:
                VX = rand() & NN;
                break;

            case 0xD: {
                int x = VX % 64;
                int y = VY % 32;
                VF = 0;

                for (int i = 0; i < N; i++) {
                    x = VX % 64;
                    uint8_t nthSpriteRow = ram[indexReg + i];
                    for (int j = 7; j >= 0; j--) {
                        bool bit = (nthSpriteRow & (1 << j)) >> j;
                        if (bit) {
                            VF |= dispBuf[x][y];
                            dispBuf[x][y] = !dispBuf[x][y];
                        }
                        x++;
                        if (x > 63) break;
                    }
                    y++;
                }

                break;
            }

            case 0xE:
                

            #if DEBUG
            default:
                printf("Instruction not implemented\n");
            #endif
        }


        // Display
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 64; x++) {
                if (dispBuf[x][y])
                    printf("|█");
                else
                    printf("| ");
            }
            printf("|\n");
        }


        // Wait for next tick
        sleep(1);
        printf("\e[1;1H\e[2J"); // clear console
    }


    return 0;
}
