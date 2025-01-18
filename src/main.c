#include <stdio.h>
#include <stdlib.h>  // Only needed for rand()

#include "core.h"

#define SDL_MAIN_USE_CALLBACKS true
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define VERSION "0.1.0"
#define PROG_NAME "cchip8"
#define APP_NAME "CChip8"

static SDL_Window* gp_window = NULL;
static SDL_Renderer* gp_renderer = NULL;

// Bit flags
uint16_t g_heldKeys = 0;

#define OFF_COLOUR 0x8f9185
#define ON_COLOUR 0x111d2b
#define DISPLAY_FREQ 60
uint64_t g_dispTick = 0;

uint64_t g_emulationFreq = 700;
uint64_t g_emulTick = 0;


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

    if (!SDL_CreateWindowAndRenderer(APP_NAME,
                                     640,
                                     320,
                                     SDL_WINDOW_RESIZABLE,
                                     &gp_window,
                                     &gp_renderer) ||
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


    *appstate = core_init();
    MachineState* machineState = *appstate;

    // Load program ROM
    FILE* romFile = fopen(argv[1], "rb");
    if (romFile == NULL) {
        printf("ROM file could not be opened\n");
        return SDL_APP_FAILURE;
    }
    core_loadROM(machineState, romFile);
    fclose(romFile);


#if DEBUG
    // Dump RAM to the console
    printf("ADDR: DATA");
    for (int i = 0; i < 0x1000; i++) {
        if (i % 16 == 0) printf("\n%04X: ", i);
        printf("0x%02X ", machineState->ram[i]);
    }
    printf("\n\n");
#endif

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    if (event->type == SDL_EVENT_KEY_DOWN &&
        event->key.scancode == SDL_SCANCODE_MINUS)
        g_emulationFreq -= 100;
    if (event->type == SDL_EVENT_KEY_DOWN &&
        event->key.scancode == SDL_SCANCODE_EQUALS)
        g_emulationFreq += 100;

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


#define V0 state->varRegs[0x0]
#define V1 state->varRegs[0x1]
#define V2 state->varRegs[0x2]
#define V3 state->varRegs[0x3]
#define V4 state->varRegs[0x4]
#define V5 state->varRegs[0x5]
#define V6 state->varRegs[0x6]
#define V7 state->varRegs[0x7]
#define V8 state->varRegs[0x8]
#define V9 state->varRegs[0x9]
#define VA state->varRegs[0xA]
#define VB state->varRegs[0xB]
#define VC state->varRegs[0xC]
#define VD state->varRegs[0xD]
#define VE state->varRegs[0xE]
#define VF state->varRegs[0xF]
#define VX state->varRegs[X]
#define VY state->varRegs[Y]

SDL_AppResult SDL_AppIterate(void* appstate) {
    MachineState* state = appstate;


    uint64_t deltaT = SDL_GetTicks() - g_emulTick;
    if (deltaT < (1000.0 / g_emulationFreq))
        return SDL_APP_CONTINUE;
    else
        g_emulTick = SDL_GetTicks();
#if DEBUG
    printf("\x1b[2J\x1b[H");
    printf("Emulation Frequency: %d Hz\n", g_emulationFreq);
#endif


    /* Fetch */
    uint16_t instruction = (state->ram[state->programCounter] << 8) +
                           state->ram[state->programCounter + 1];
    state->programCounter += 2;
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
//     // Stop execution once in an infinite loop
//     if (NNN == state->programCounter - 2) return SDL_APP_SUCCESS;
// #endif

// Display machine state
#if DEBUG
    printf("PC: 0x%04X\n", state->programCounter - 2);
    printf("Stack:\n");
    for (int i = 0; i < 16; i++) printf("    0x%04X,\n", state->stack[i]);
    printf("SP: %d\n", state->stackIdx);
    printf("I : 0x%04X\n", state->indexReg);
    printf("V : ");
    for (int i = 0; i < 16; i++) printf("0x%02X, ", state->varRegs[i]);
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
                            memset(state->dispBuf, 0, sizeof(state->dispBuf));
                            updateDisp = true;
                            break;

                        case 0xE:
                            state->programCounter = pop(state);
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
            state->programCounter = NNN;
            break;

        case 0x2:
            push(state, state->programCounter);
            state->programCounter = NNN;
            break;

        case 0x3:
            if (VX == NN) state->programCounter += 2;
            break;

        case 0x4:
            if (VX != NN) state->programCounter += 2;
            break;

        case 0x5:
            if (VX == VY) state->programCounter += 2;
            break;

        case 0x9:
            if (VX != VY) state->programCounter += 2;
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
            state->indexReg = NNN;
            break;

        case 0xB:
            state->programCounter = NNN + V0;
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
                uint8_t nthSpriteRow = state->ram[state->indexReg + i];
                for (int j = 7; j >= 0; j--) {
                    bool bit = (nthSpriteRow & (1 << j)) >> j;
                    if (bit) {
                        VF |= state->dispBuf[x][y];
                        state->dispBuf[x][y] = !state->dispBuf[x][y];
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
                    if ((g_heldKeys >> VX) & 0b1) state->programCounter += 2;
                    break;

                case 0xA1:
                    if (!((g_heldKeys >> VX) & 0b1)) state->programCounter += 2;
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
                    VX = state->delayTimer;
                    break;

                case 0x15:
                    state->delayTimer = VX;
                    break;

                case 0x18:
                    state->soundTimer = VX;
                    break;

                case 0x1E:
                    state->indexReg += VX;
                    if (state->indexReg > 0x1000)
                        VF = 1;  // Amiga specific behaviour
                    break;

                case 0x0A:
                    if (g_heldKeys == 0)
                        state->programCounter -= 2;
                    else
                        for (int i = 0; i < 16; i++)
                            if (g_heldKeys >> i & 0b1) VX = i;

                    break;

                case 0x29:
                    state->indexReg = 0x50 + (VX & 0xF) * 5;
                    break;

                case 0x33:
                    state->ram[state->indexReg + 2] = (VX / 1) % 10;
                    state->ram[state->indexReg + 1] = (VX / 10) % 10;
                    state->ram[state->indexReg + 0] = (VX / 100) % 10;
                    break;

                case 0x55:
                    for (int i = 0; i <= X; i++)
                        state->ram[state->indexReg + i] = state->varRegs[i];
                    break;

                case 0x65:
                    for (int i = 0; i <= X; i++)
                        state->varRegs[i] = state->ram[state->indexReg + i];
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
        if ((SDL_GetTicks() - g_dispTick) > (1000.0 / DISPLAY_FREQ)) {
            g_dispTick = SDL_GetTicks();
            state->delayTimer--;
        }

        // Clear the screen to black
        SDL_SetRenderDrawColor(gp_renderer,
                               OFF_COLOUR >> 16 & 0xFF,
                               OFF_COLOUR >> 8 & 0xFF,
                               OFF_COLOUR >> 8 & 0xFF,
                               SDL_ALPHA_OPAQUE);
        SDL_RenderClear(gp_renderer);


        // Render points
        SDL_SetRenderDrawColor(gp_renderer,
                               ON_COLOUR >> 16 & 0xFF,
                               ON_COLOUR >> 8 & 0xFF,
                               ON_COLOUR >> 8 & 0xFF,
                               SDL_ALPHA_OPAQUE);
        for (int y = 0; y < 32; y++)
            for (int x = 0; x < 64; x++)
                if (state->dispBuf[x][y]) SDL_RenderPoint(gp_renderer, x, y);

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
