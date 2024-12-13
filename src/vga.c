#include <string.h>
#include <stdio.h>
#include <vga.h>

// Standard VGA color palette (16 colors)
static const SDL_Color vga_palette[16] = {
    {0,   0,   0,   255},  // 0: Black
    {0,   0,   170, 255},  // 1: Blue
    {0,   170, 0,   255},  // 2: Green
    {0,   170, 170, 255},  // 3: Cyan
    {170, 0,   0,   255},  // 4: Red
    {170, 0,   170, 255},  // 5: Magenta
    {170, 85,  0,   255},  // 6: Brown
    {170, 170, 170, 255},  // 7: Light Gray
    {85,  85,  85,  255},  // 8: Dark Gray
    {85,  85,  255, 255},  // 9: Light Blue
    {85,  255, 85,  255},  // 10: Light Green
    {85,  255, 255, 255},  // 11: Light Cyan
    {255, 85,  85,  255},  // 12: Light Red
    {255, 85,  255, 255},  // 13: Light Magenta
    {255, 255, 85,  255},  // 14: Yellow
    {255, 255, 255, 255}   // 15: White
};

int vga_init(VGA* vga) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 0;
    }

    if (TTF_Init() < 0) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
        return 0;
    }

    vga->window = SDL_CreateWindow("VM VGA Display",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);

    if (!vga->window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 0;
    }

    vga->renderer = SDL_CreateRenderer(vga->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!vga->renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return 0;
    }

    vga->font = TTF_OpenFont("fonts/Perfect_DOS_VGA.ttf", CHAR_HEIGHT);

    memset(vga->screen, 0, sizeof(vga->screen));
    vga->cursor_x = 0;
    vga->cursor_y = 0;

    return 1;
}

void vga_scroll_up(VGA* vga) {
    // Move all lines up one
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        memcpy(&vga->screen[y], &vga->screen[y + 1], sizeof(VGACell) * VGA_WIDTH);
    }
    // Clear bottom line
    memset(&vga->screen[VGA_HEIGHT - 1], 0, sizeof(VGACell) * VGA_WIDTH);
}

void vga_update(VGA* vga) {
    SDL_SetRenderDrawColor(vga->renderer, 0, 0, 0, 255);
    SDL_RenderClear(vga->renderer);

    char text[2] = {0, 0};
    SDL_Surface* surface;
    SDL_Texture* texture;
    SDL_Rect destRect = {0, 0, CHAR_WIDTH, CHAR_HEIGHT};

    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGACell* cell = &vga->screen[y][x];
            text[0] = cell->character;

            if (text[0] != 0) {
                uint8_t fg = cell->attribute & 0x0F;
                uint8_t bg = (cell->attribute >> 4) & 0x0F;
                
                SDL_Color fgColor = vga_palette[fg];
                SDL_Color bgColor = vga_palette[bg];

                surface = TTF_RenderText_Shaded(vga->font, text, fgColor, bgColor);
                if (surface) {
                    texture = SDL_CreateTextureFromSurface(vga->renderer, surface);
                    destRect.x = x * CHAR_WIDTH;
                    destRect.y = y * CHAR_HEIGHT;
                    SDL_RenderCopy(vga->renderer, texture, NULL, &destRect);
                    SDL_DestroyTexture(texture);
                    SDL_FreeSurface(surface);
                }
            }
        }
    }

    SDL_RenderPresent(vga->renderer);
}

void vga_write_memory(VGA* vga, uint32_t address, uint8_t value) {
    if (address >= VGA_MEMORY_START && address < VGA_MEMORY_START + VGA_MEMORY_SIZE) {
        uint32_t offset = address - VGA_MEMORY_START;
        int cell_index = offset / 2;
        int y = cell_index / VGA_WIDTH;
        int x = cell_index % VGA_WIDTH;
        
        if (offset % 2 == 0) {
            vga->screen[y][x].character = value;
        } else {
            vga->screen[y][x].attribute = value;
        }
    }
}

void vga_cleanup(VGA* vga) {
    if (vga->font) TTF_CloseFont(vga->font);
    if (vga->renderer) SDL_DestroyRenderer(vga->renderer);
    if (vga->window) SDL_DestroyWindow(vga->window);
    TTF_Quit();
    SDL_Quit();
}