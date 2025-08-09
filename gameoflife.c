/*
 * Game of Life
 * Made for WHY2025 Badge
 * By Edwin Martin 2025
 * License: MIT
 */

#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define STEP_RATE_IN_MILLISECONDS  250
#define BLOCK_SIZE_IN_PIXELS 24
#define SDL_WINDOW_WIDTH           (BLOCK_SIZE_IN_PIXELS * GAME_WIDTH)
#define SDL_WINDOW_HEIGHT          (BLOCK_SIZE_IN_PIXELS * GAME_HEIGHT)

#define GAME_WIDTH  24U
#define GAME_HEIGHT 18U

typedef struct {
    unsigned char cells[GAME_WIDTH * GAME_HEIGHT];
    unsigned char newCells[GAME_WIDTH * GAME_HEIGHT];
} GolContext;

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    GolContext  gol_ctx;
    Uint64        last_step;
} AppState;

static void set_rect_xy_(SDL_FRect *r, short x, short y) {
    r->x = (float)(x * BLOCK_SIZE_IN_PIXELS);
    r->y = (float)(y * BLOCK_SIZE_IN_PIXELS);
}

void gol_initialize(GolContext *ctx) {
    SDL_zeroa(ctx->cells);

    for ( int i = 0; i < GAME_WIDTH * GAME_HEIGHT / 2; i++) {
        int const n = (int)SDL_rand(GAME_WIDTH * GAME_HEIGHT);
        ctx->cells[n] = 1;
    }
}

int getAtPos(GolContext *ctx, int x, int y) {
    if (x < 0 || x >= GAME_WIDTH || y < 0 || y >= GAME_HEIGHT) {
        return 0;
    }

    return ctx->cells[x + y * GAME_WIDTH];
}

void gol_step(GolContext *ctx) {
    int x, y;

    SDL_zeroa(ctx->newCells);

    for (x = 0; x < GAME_WIDTH; x++) {
        for (y = 0; y < GAME_HEIGHT; y++) {
            int neighbors = getAtPos(ctx, x - 1, y - 1)
             + getAtPos(ctx, x, y - 1)
             + getAtPos(ctx, x + 1, y - 1)
             + getAtPos(ctx, x - 1, y)
             + getAtPos(ctx, x + 1, y)
             + getAtPos(ctx, x - 1, y + 1)
             + getAtPos(ctx, x, y + 1)
             + getAtPos(ctx, x + 1, y + 1);

             if (neighbors == 2) {
                ctx->newCells[x + y * GAME_WIDTH] = getAtPos(ctx, x, y);
             }
             if (neighbors == 3) {
                ctx->newCells[x + y * GAME_WIDTH] = 1;
             } 
        }
    }

    for ( int i = 0; i < GAME_WIDTH * GAME_HEIGHT; i++) {
        ctx->cells[i] = ctx->newCells[i];
    }
}

static SDL_AppResult handle_key_event_(GolContext *ctx, SDL_Scancode key_code) {
    switch (key_code) {
        /* Quit. */
        case SDL_SCANCODE_ESCAPE:
        case SDL_SCANCODE_Q: return SDL_APP_SUCCESS;
        /* Restart the game as if the program was launched. */
        case SDL_SCANCODE_R: gol_initialize(ctx); break;

        default: break;
    }
    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState     *as  = (AppState *)appstate;
    GolContext *ctx = &as->gol_ctx;
    Uint64 const  now = SDL_GetTicks();
    SDL_FRect     r;
    unsigned      i;
    unsigned      j;
    int           ct;

    // run game logic if we're at or past the time to run it.
    // if we're _really_ behind the time to run it, run it
    // several times.
    while ((now - as->last_step) >= STEP_RATE_IN_MILLISECONDS) {
        gol_step(ctx);
        as->last_step += STEP_RATE_IN_MILLISECONDS;
    }

    r.w = r.h = BLOCK_SIZE_IN_PIXELS - 1;
    SDL_SetRenderDrawColor(as->renderer, 96, 96, 96, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(as->renderer);
    SDL_SetRenderDrawColor(as->renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);

    for (i = 0; i < GAME_WIDTH; i++) {
        for (j = 0; j < GAME_HEIGHT; j++) {
            if (ctx->cells[i + j * GAME_WIDTH]) {
                set_rect_xy_(&r, i, j);
                SDL_RenderFillRect(as->renderer, &r);
            }
        }
    }
    SDL_RenderFillRect(as->renderer, &r);
    SDL_RenderPresent(as->renderer);
    return SDL_APP_CONTINUE;
}

static const struct {
    char const *key;
    char const *value;
} extended_metadata[] = {
    {SDL_PROP_APP_METADATA_URL_STRING, "https://badge.why2025.org/"},
    {SDL_PROP_APP_METADATA_CREATOR_STRING, "Badge team"},
    {SDL_PROP_APP_METADATA_COPYRIGHT_STRING, "MIT"},
    {SDL_PROP_APP_METADATA_TYPE_STRING, "game"}
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    size_t i;

    if (!SDL_SetAppMetadata("Game of life", "1.0", "org.bitstorm.gameoflife")) {
        return SDL_APP_FAILURE;
    }

    for (i = 0; i < SDL_arraysize(extended_metadata); i++) {
        if (!SDL_SetAppMetadataProperty(extended_metadata[i].key, extended_metadata[i].value)) {
            return SDL_APP_FAILURE;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    AppState *as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as) {
        return SDL_APP_FAILURE;
    }

    *appstate = as;

    // Create window first
    as->window = SDL_CreateWindow("Game of Life", SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, 0);
    if (!as->window) {
        printf("Failed to create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Check display capabilities
    SDL_DisplayID          display      = SDL_GetDisplayForWindow(as->window);
    SDL_DisplayMode const *current_mode = SDL_GetCurrentDisplayMode(display);
    if (current_mode) {
        printf(
            "Current display mode: %dx%d @%.2fHz, format: %s",
            current_mode->w,
            current_mode->h,
            current_mode->refresh_rate,
            SDL_GetPixelFormatName(current_mode->format)
        );
    }

    // Create renderer
    as->renderer = SDL_CreateRenderer(as->window, NULL);
    if (!as->renderer) {
        printf("Failed to create renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Check renderer properties
    SDL_PropertiesID props = SDL_GetRendererProperties(as->renderer);
    if (props) {
        char const *name = SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "Unknown");
        printf("Renderer: %s", name);

        SDL_PixelFormat const *formats =
            (SDL_PixelFormat const *)SDL_GetPointerProperty(props, SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER, NULL);
        if (formats) {
            printf("Supported texture formats:");
            for (int j = 0; formats[j] != SDL_PIXELFORMAT_UNKNOWN; j++) {
                printf("  Format %d: %s", j, SDL_GetPixelFormatName(formats[j]));
            }
        }
    }

    gol_initialize(&as->gol_ctx);
    as->last_step = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    GolContext *ctx = &((AppState *)appstate)->gol_ctx;
    switch (event->type) {
        case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
        case SDL_EVENT_KEY_DOWN: return handle_key_event_(ctx, event->key.scancode);
        default: break;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    if (appstate != NULL) {
        AppState *as = (AppState *)appstate;
        SDL_DestroyRenderer(as->renderer);
        SDL_DestroyWindow(as->window);
        SDL_free(as);
    }
}
