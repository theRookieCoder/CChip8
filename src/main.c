#include <stdio.h>
#include <stdlib.h>

#define SDL_MAIN_USE_CALLBACKS true
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "core.h"

#define VERSION "0.1.0"
#define PROG_NAME "cchip8"
#define APP_NAME "CChip8"


const SDL_Scancode KEYMAP[16] = {
    SDL_SCANCODE_X,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_E,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S,
    SDL_SCANCODE_D,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_C,
    SDL_SCANCODE_4,
    SDL_SCANCODE_R,
    SDL_SCANCODE_F,
    SDL_SCANCODE_V,
};

// How long to buffer a key press in 60 Hz time periods (16.67 ms)
#define KEY_BUFFER_DUR 1

// How many 60 Hz time periods a key has been held for
uint64_t g_keyRepeat[16] = {};
// Whether the key has been released but is still being buffered
bool g_keyReleased[16] = {};

uint16_t heldKeys() {
    uint16_t heldKeys = 0;

    for (int i = 0; i < 16; i++) {
        if (g_keyRepeat[i] > 0) heldKeys |= 0b1 << i;
        if (g_keyReleased[i]) {
            g_keyRepeat[i] = 0;
            g_keyReleased[i] = false;
        }
    }

    return heldKeys;
}


static SDL_Window* gp_window = NULL;
static SDL_Renderer* gp_renderer = NULL;

#define OFF_COLOUR 0x8f9185
#define ON_COLOUR 0x111d2b
bool g_windowNeedsRedraw = false;
uint64_t g_dispTick = 0;

uint64_t g_emulationFreq = 500;
uint64_t g_emulTick = 0;
bool g_runEmul = true;


bool g_displayBuffer[64][32];
bool getPixel(uint8_t x, uint8_t y) { return g_displayBuffer[x % 64][y % 32]; }
void togglePixel(uint8_t x, uint8_t y) {
    g_displayBuffer[x % 64][y % 32] = !g_displayBuffer[x % 64][y % 32];
}
void clearDisplay() { memset(g_displayBuffer, 0, sizeof(g_displayBuffer)); }


void sigIllHandler() {}


SDL_AppResult SDL_AppInit(void** pp_appstate, int argc, char* p_argv[]) {
    printf("%s version %s\n\n", PROG_NAME, VERSION);
    SDL_SetAppMetadata(APP_NAME, VERSION, "io.github.theRookieCoder.CChip8");

    if (argc < 2) {
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

    // if (!SDL_SetRenderVSync(gp_renderer, 1)) {
    //     SDL_Log("Couldn't set VSync: %s", SDL_GetError());
    // }

    // Clear the screen to black
    SDL_SetRenderDrawColor(gp_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(gp_renderer);
    SDL_RenderPresent(gp_renderer);


    static MachineState machineState = {};
    *pp_appstate = &machineState;
    core_init(&machineState,
              NULL,
              NULL,
              &heldKeys,
              &getPixel,
              &togglePixel,
              &clearDisplay,
              &sigIllHandler);

    // Load program ROM
    FILE* romFile = fopen(p_argv[1], "rb");
    if (romFile == NULL) {
        SDL_Log("ROM file could not be opened");
        return SDL_APP_FAILURE;
    }
    int read = fread(&machineState.ram[0x0200],
                     sizeof(*(machineState.ram)),
                     sizeof(machineState.ram) - 0x0200,
                     romFile);
#if DEBUG
    printf("Loaded %i bytes into machine state's RAM.\n", read);
#endif
    fclose(romFile);


#if DEBUG
    // Dump RAM to the console
    printf("ADDR: DATA");
    for (int i = 0; i < 0x1000; i++) {
        if (i % 16 == 0) printf("\n%04X: ", i);
        printf("0x%02X ", machineState.ram[i]);
    }
    printf("\n\n");
#endif

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void*, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;


    if (event->type >= SDL_EVENT_WINDOW_FIRST &&
        event->type <= SDL_EVENT_WINDOW_LAST)
        g_windowNeedsRedraw = true;


    if (event->type == SDL_EVENT_KEY_DOWN &&
        event->key.scancode == SDL_SCANCODE_SPACE)
        g_runEmul = !g_runEmul;

    if (event->type == SDL_EVENT_KEY_DOWN &&
        event->key.scancode == SDL_SCANCODE_MINUS)
        g_emulationFreq -= 100;
    if (event->type == SDL_EVENT_KEY_DOWN &&
        event->key.scancode == SDL_SCANCODE_EQUALS)
        g_emulationFreq += 100;


    if (event->type == SDL_EVENT_KEY_DOWN)
        for (int i = 0; i < 16; i++)
            if (event->key.scancode == KEYMAP[i]) {
                g_keyRepeat[i]++;
                g_keyReleased[i] = false;
            }

    if (event->type == SDL_EVENT_KEY_UP)
        for (int i = 0; i < 16; i++)
            if (event->key.scancode == KEYMAP[i]) g_keyReleased[i] = true;


    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void* p_appstate) {
    MachineState* p_machineState = p_appstate;

    bool emulUpdatedDisp = false;
    uint64_t currentTicks = SDL_GetTicksNS();

    /* CORE TICKING */

    // Tick the core if needed
    if (g_runEmul &&
        (currentTicks - g_emulTick) > (1000000000 / g_emulationFreq)) {
#if DEBUG
        printf("\x1b[2J\x1b[H");
        printf("Emulation frequency  : %lu Hz\n", g_emulationFreq);
        printf("Emulation time period: %g ms\n",
               ((double)currentTicks - g_emulTick) / 1000000);

        uint16_t held = 0;
        for (int i = 0; i < 16; i++)
            if (g_keyRepeat[i]) held |= 0b1 << i;
        printf("Held keys: %016B\n", held);
        printf("           FEDCBA9876543210\n\n");
#endif

        g_emulTick = currentTicks;

        emulUpdatedDisp = core_tick(p_machineState);
    };

    // Tick the delay and sound timers at 60 Hz
    // Increment the key repeat
    if (g_runEmul && (currentTicks - g_dispTick) > (1000000000 / 60)) {
        g_dispTick = currentTicks;
        core_timerTick(p_machineState);

        for (int i = 0; i < 16; i++) {
            // If the key has been released and exceeded the buffer duration
            if (g_keyRepeat[i] > KEY_BUFFER_DUR && g_keyReleased[i]) {
                g_keyRepeat[i] = 0;
                g_keyReleased[i] = false;
            }
            if (g_keyRepeat[i] > 0) g_keyRepeat[i]++;
        }
    }


    /* DISPLAY */

    // Update the display when the display buffer is updated or window is
    // resized
    if (emulUpdatedDisp || g_windowNeedsRedraw) {
        g_windowNeedsRedraw = false;

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
                if (g_displayBuffer[x][y]) SDL_RenderPoint(gp_renderer, x, y);

        // Present the screen
        SDL_RenderPresent(gp_renderer);
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void*, SDL_AppResult result) {
#if DEBUG
    // Some buffer time to have a look at the display if something goes wrong
    if (result == SDL_APP_FAILURE) SDL_Delay(3000);
#endif
}
