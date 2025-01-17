#include <stdio.h>
#include <stdlib.h>  // Only needed for rand()

#define SDL_MAIN_USE_CALLBACKS true
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define VERSION "0.0.1"
#define PROG_NAME "cchip8"
#define APP_NAME "CChip8"

static SDL_Window* gp_window = NULL;
static SDL_Renderer* gp_renderer = NULL;

// Bit flags
uint16_t g_heldKeys = 0;

#define DISPLAY_FREQ 60
uint64_t g_dispTick = 0;

#define EMULATION_FREQ 70
uint64_t g_emulTick = 0;

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

uint16_t g_stack[16] = {};
uint8_t g_stackIdx = 0;

uint8_t g_ram[4096] = {};

bool g_dispBuf[64][32] = {};

uint8_t g_delayTimer = 0;
uint8_t g_soundTimer = 0;

uint16_t g_programCounter = 0x200;
uint16_t g_indexReg = 0;

uint8_t g_varRegs[16] = {};
#define V0 g_varRegs[0x0]
#define V1 g_varRegs[0x1]
#define V2 g_varRegs[0x2]
#define V3 g_varRegs[0x3]
#define V4 g_varRegs[0x4]
#define V5 g_varRegs[0x5]
#define V6 g_varRegs[0x6]
#define V7 g_varRegs[0x7]
#define V8 g_varRegs[0x8]
#define V9 g_varRegs[0x9]
#define VA g_varRegs[0xA]
#define VB g_varRegs[0xB]
#define VC g_varRegs[0xC]
#define VD g_varRegs[0xD]
#define VE g_varRegs[0xE]
#define VF g_varRegs[0xF]
#define VX g_varRegs[X]
#define VY g_varRegs[Y]

void push(uint16_t val) {
    if (g_stackIdx > 16) printf("Stack overflow!\n");

    g_stack[g_stackIdx++] = val;
}

uint16_t pop() { return g_stack[--g_stackIdx]; }

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    printf("%s version %s\n\n", PROG_NAME, VERSION);
    SDL_SetAppMetadata(APP_NAME, VERSION, "io.github.theRookieCoder.CChip8");

    if (argc != 2) {
        printf("Usage: cchip8 rom_file\n");
        return SDL_APP_FAILURE;
    }


    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer(
            APP_NAME, 640, 320, 0, &gp_window, &gp_renderer) ||
        gp_window == NULL || gp_renderer == NULL) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_SetRenderLogicalPresentation(
            gp_renderer, 64, 32, SDL_LOGICAL_PRESENTATION_STRETCH)) {
        SDL_Log("Couldn't set logical render resolution: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Clear the screen to black
    SDL_SetRenderDrawColor(gp_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(gp_renderer);
    SDL_RenderPresent(gp_renderer);


    // Load font
    for (int i = 0; i < 16 * 5; i++) g_ram[0x50 + i] = FONT[i];

    // Load program ROM
    FILE* rom_file = fopen(argv[1], "rb");

    if (rom_file == NULL) {
        printf("ROM file could not be opened\n");
        return SDL_APP_FAILURE;
    }

    int i = 0;
    int written = 0;
    while ((written = fread(&g_ram[0x200 + i], sizeof *g_ram, 1, rom_file)) > 0)
        i += written;

    fclose(rom_file);

#if DEBUG
    // Dump RAM to the console
    printf("ADDR: DATA");
    for (int i = 0; i < 0x1000; i++) {
        if (i % 16 == 0) printf("\n%04X: ", i);
        printf("0x%02X ", g_ram[i]);
    }
    printf("\n");
#endif

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    if (event->type == SDL_EVENT_KEY_DOWN) switch (event->key.scancode) {
            case SDL_SCANCODE_1:
                g_heldKeys |= 0b1 << 0x1;
                break;
            case SDL_SCANCODE_2:
                g_heldKeys |= 0b1 << 0x2;
                break;
            case SDL_SCANCODE_3:
                g_heldKeys |= 0b1 << 0x3;
                break;
            case SDL_SCANCODE_4:
                g_heldKeys |= 0b1 << 0xC;
                break;
            case SDL_SCANCODE_Q:
                g_heldKeys |= 0b1 << 0x4;
                break;
            case SDL_SCANCODE_W:
                g_heldKeys |= 0b1 << 0x5;
                break;
            case SDL_SCANCODE_E:
                g_heldKeys |= 0b1 << 0x6;
                break;
            case SDL_SCANCODE_R:
                g_heldKeys |= 0b1 << 0xD;
                break;
            case SDL_SCANCODE_A:
                g_heldKeys |= 0b1 << 0x7;
                break;
            case SDL_SCANCODE_S:
                g_heldKeys |= 0b1 << 0x8;
                break;
            case SDL_SCANCODE_D:
                g_heldKeys |= 0b1 << 0x9;
                break;
            case SDL_SCANCODE_F:
                g_heldKeys |= 0b1 << 0xE;
                break;
            case SDL_SCANCODE_Z:
                g_heldKeys |= 0b1 << 0xA;
                break;
            case SDL_SCANCODE_X:
                g_heldKeys |= 0b1 << 0x0;
                break;
            case SDL_SCANCODE_C:
                g_heldKeys |= 0b1 << 0xB;
                break;
            case SDL_SCANCODE_V:
                g_heldKeys |= 0b1 << 0xF;
                break;
        };

    if (event->type == SDL_EVENT_KEY_UP) switch (event->key.scancode) {
            case SDL_SCANCODE_1:
                g_heldKeys &= 0b0 << 0x1;
                break;
            case SDL_SCANCODE_2:
                g_heldKeys &= 0b0 << 0x2;
                break;
            case SDL_SCANCODE_3:
                g_heldKeys &= 0b0 << 0x3;
                break;
            case SDL_SCANCODE_4:
                g_heldKeys &= 0b0 << 0xC;
                break;
            case SDL_SCANCODE_Q:
                g_heldKeys &= 0b0 << 0x4;
                break;
            case SDL_SCANCODE_W:
                g_heldKeys &= 0b0 << 0x5;
                break;
            case SDL_SCANCODE_E:
                g_heldKeys &= 0b0 << 0x6;
                break;
            case SDL_SCANCODE_R:
                g_heldKeys &= 0b0 << 0xD;
                break;
            case SDL_SCANCODE_A:
                g_heldKeys &= 0b0 << 0x7;
                break;
            case SDL_SCANCODE_S:
                g_heldKeys &= 0b0 << 0x8;
                break;
            case SDL_SCANCODE_D:
                g_heldKeys &= 0b0 << 0x9;
                break;
            case SDL_SCANCODE_F:
                g_heldKeys &= 0b0 << 0xE;
                break;
            case SDL_SCANCODE_Z:
                g_heldKeys &= 0b0 << 0xA;
                break;
            case SDL_SCANCODE_X:
                g_heldKeys &= 0b0 << 0x0;
                break;
            case SDL_SCANCODE_C:
                g_heldKeys &= 0b0 << 0xB;
                break;
            case SDL_SCANCODE_V:
                g_heldKeys &= 0b0 << 0xF;
                break;
        };

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    if ((SDL_GetTicks() - g_emulTick) < (1000.0 / EMULATION_FREQ))
        return SDL_APP_CONTINUE;
    else
        g_emulTick = SDL_GetTicks();


    /* Fetch */
    uint16_t instruction =
        (g_ram[g_programCounter] << 8) + g_ram[g_programCounter + 1];
    g_programCounter += 2;
#if DEBUG
    printf("\nInstruction: %04X\n", instruction);
    printf("Held keys  : %016B\n", g_heldKeys);
    printf("             FEDCBA9876543210\n");
#endif


    /* Decode */
#define X ((instruction & 0x0F00) >> 8)
#define Y ((instruction & 0x00F0) >> 4)
#define N (instruction & 0x000F)
#define NN (instruction & 0x00FF)
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
    printf("PC: 0x%04X\n", g_programCounter);
    printf("Stack:\n");
    for (int i = 0; i < 16; i++) printf("    0x%04X,\n", g_stack[i]);
    printf("SP: %d\n", g_stackIdx);
    printf("I : 0x%04X\n", g_indexReg);
    printf("V : ");
    for (int i = 0; i < 16; i++) printf("0x%02X, ", g_varRegs[i]);
    printf("\n");
    printf("VX: 0x%02X\n", VX);
    printf("VY: 0x%02X\n", VY);
#endif


    bool updateDisp = false;

    /* Execute */
    switch ((instruction & 0xF000) >> 12) {
        case 0x0: {
            switch (Y) {
                case 0xE: {
                    switch (N) {
                        case 0x0:
                            memset(g_dispBuf, 0, sizeof(g_dispBuf));
                            break;

                        case 0xE:
                            g_programCounter = pop();
                            break;
#if DEBUG
                        default:
                            printf("Instruction not implemented\n");
                            return SDL_APP_FAILURE;
#endif
                    }

                    break;
                }
#if DEBUG
                default:
                    printf("Instruction not implemented\n");
                    return SDL_APP_FAILURE;
#endif
            }

            break;
        }

        case 0x1:
            g_programCounter = NNN;
            break;

        case 0x2:
            push(g_programCounter);
            g_programCounter = NNN;
            break;

        case 0x3:
            if (VX == NN) g_programCounter += 2;
            break;

        case 0x4:
            if (VX != NN) g_programCounter += 2;
            break;

        case 0x5:
            if (VX == VY) g_programCounter += 2;
            break;

        case 0x9:
            if (VX != VY) g_programCounter += 2;
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

                case 0x4:
                    if (VX == 255)
                        VF = 1;
                    else
                        VF = 0;

                    VX += VY;
                    break;

                case 0x5:
                    VF = VX > VY;
                    VX = VX - VY;
                    break;

                case 0x7:
                    VF = VY > VX;
                    VX = VY - VX;
                    break;

                case 0x6:
#if !SCHIP
                    VX = VY;
#endif
                    VF = VX & 0b00000001;
                    VX = VX >> 1;
                    break;

                case 0xE:
#if !SCHIP
                    VX = VY;
#endif
                    VF = (VX & 0b10000000) >> 7;
                    VX = VX << 1;
                    break;
#if DEBUG
                default:
                    printf("Instruction not implemented\n");
                    return SDL_APP_FAILURE;
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
            g_indexReg = NNN;
            break;

        case 0xB:
            g_programCounter = NNN + V0;
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
                uint8_t nthSpriteRow = g_ram[g_indexReg + i];
                for (int j = 7; j >= 0; j--) {
                    bool bit = (nthSpriteRow & (1 << j)) >> j;
                    if (bit) {
                        VF |= g_dispBuf[x][y];
                        g_dispBuf[x][y] = !g_dispBuf[x][y];
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
                    if ((g_heldKeys >> VX) & 0b1) g_programCounter += 2;
                    break;

                case 0xA1:
                    if (!((g_heldKeys >> VX) & 0b1)) g_programCounter += 2;
                    break;
#if DEBUG
                default:
                    printf("Instruction not implemented\n");
                    return SDL_APP_FAILURE;
#endif
            }

            break;
        }

        case 0xF: {
            switch (NN) {
                case 0x07:
                    VX = g_delayTimer;
                    break;

                case 0x15:
                    g_delayTimer = VX;
                    break;

                case 0x18:
                    g_soundTimer = VX;
                    break;

                case 0x1E:
                    g_indexReg += VX;
                    if (g_indexReg > 0x1000)
                        VF = 1;  // Amiga specific behaviour
                    break;

                case 0x0A:
                    if (g_heldKeys == 0)
                        g_programCounter -= 2;
                    else
                        for (int i = 0; i < 16; i++)
                            if (g_heldKeys >> i & 0b1) VX = i;

                    break;

                case 0x29:
                    g_indexReg = 0x50 + (VX & 0xF);
                    break;

                case 0x33:
                    g_ram[g_indexReg + 2] = (VX / 1) % 10;
                    g_ram[g_indexReg + 1] = (VX / 10) % 10;
                    g_ram[g_indexReg + 0] = (VX / 100) % 10;
                    break;

                case 0x55:
                    for (int i = 0; i <= X; i++)
                        g_ram[g_indexReg + i] = g_varRegs[i];
                    break;

                case 0x65:
                    for (int i = 0; i <= X; i++)
                        g_varRegs[i] = g_ram[g_indexReg + i];
                    break;
#if DEBUG
                default:
                    printf("Instruction not implemented\n");
                    return SDL_APP_FAILURE;
#endif
            }

            break;
        }

#if DEBUG
        default:
            printf("Instruction not implemented\n");
            return SDL_APP_FAILURE;
#endif
    }


    /* Display */

    // Update the display when the display buffer is updated or at 60Hz
    if (updateDisp || (SDL_GetTicks() - g_dispTick) > (1000.0 / DISPLAY_FREQ)) {
        g_dispTick = SDL_GetTicks();
        g_delayTimer--;  // Decrement every frame

        // Clear the screen to black
        SDL_SetRenderDrawColor(gp_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(gp_renderer);


        // Render points
        SDL_SetRenderDrawColor(gp_renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
        for (int y = 0; y < 32; y++)
            for (int x = 0; x < 64; x++)
                if (g_dispBuf[x][y]) SDL_RenderPoint(gp_renderer, x, y);

        // Present the screen
        SDL_RenderPresent(gp_renderer);
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
#if DEBUG
    if (result == SDL_APP_FAILURE) SDL_Delay(3000);
#endif
}
