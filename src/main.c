#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS true
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "core.h"

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

bool g_windowResized = false;


SDL_AppResult SDL_AppInit(void** pp_appstate, int argc, char* argv[]) {
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
                                     1280,
                                     640,
                                     SDL_WINDOW_RESIZABLE,
                                     &gp_window,
                                     &gp_renderer)) {
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


    *pp_appstate = core_init();
    MachineState* p_machineState = *pp_appstate;

    // Load program ROM
    FILE* romFile = fopen(argv[1], "rb");
    if (romFile == NULL) {
        SDL_Log("ROM file could not be opened");
        return SDL_APP_FAILURE;
    }
    core_loadROM(p_machineState, romFile);
    fclose(romFile);


#if DEBUG
    // Dump RAM to the console
    printf("ADDR: DATA");
    for (int i = 0; i < 0x1000; i++) {
        if (i % 16 == 0) printf("\n%04X: ", i);
        printf("0x%02X ", p_machineState->ram[i]);
    }
    printf("\n\n");
#endif

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void*, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    if (event->type == SDL_EVENT_WINDOW_RESIZED) g_windowResized = true;

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


uint16_t heldKeys() { return g_heldKeys; }


SDL_AppResult SDL_AppIterate(void* p_appstate) {
    MachineState* p_machineState = p_appstate;

    uint64_t deltaT = SDL_GetTicks() - g_emulTick;
    if (deltaT < (1000.0 / g_emulationFreq))
        return SDL_APP_CONTINUE;
    else
        g_emulTick = SDL_GetTicks();

#if DEBUG
    printf("\x1b[2J\x1b[H");
    printf("Emulation Frequency: %d Hz\n", g_emulationFreq);
    printf("Held keys  : %016B\n", g_heldKeys);
    printf("             FEDCBA9876543210\n\n");
#endif

    bool updateDisp = core_tick(p_machineState, &heldKeys);


    // Tick the display and sound timers at 60 Hz
    if ((SDL_GetTicks() - g_dispTick) > (1000.0 / DISPLAY_FREQ)) {
        g_dispTick = SDL_GetTicks();
        core_timerTick(p_machineState);
    }

    /* Display */

    // Update the display when the display buffer is updated or window is
    // resized
    if (updateDisp | g_windowResized) {
        g_windowResized = false;

        // Clear the screen to the off colour
        SDL_SetRenderDrawColor(gp_renderer,
                               OFF_COLOUR >> 16 & 0xFF,
                               OFF_COLOUR >> 8 & 0xFF,
                               OFF_COLOUR >> 8 & 0xFF,
                               SDL_ALPHA_OPAQUE);
        SDL_RenderClear(gp_renderer);


        // Render points with the on colour
        SDL_SetRenderDrawColor(gp_renderer,
                               ON_COLOUR >> 16 & 0xFF,
                               ON_COLOUR >> 8 & 0xFF,
                               ON_COLOUR >> 8 & 0xFF,
                               SDL_ALPHA_OPAQUE);
        for (int y = 0; y < 32; y++)
            for (int x = 0; x < 64; x++)
                if (p_machineState->dispBuf[x][y])
                    SDL_RenderPoint(gp_renderer, x, y);

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
