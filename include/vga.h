#ifndef VGA_H
#define VGA_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define CHAR_WIDTH 9
#define CHAR_HEIGHT 16
#define WINDOW_WIDTH (VGA_WIDTH * CHAR_WIDTH)
#define WINDOW_HEIGHT (VGA_HEIGHT * CHAR_HEIGHT)
#define VGA_MEMORY_START 0xB8000
#define VGA_MEMORY_SIZE (VGA_WIDTH * VGA_HEIGHT * 2)

typedef struct {
    uint8_t character;
    uint8_t attribute;
} VGACell;

typedef struct {
    VGACell screen[VGA_HEIGHT][VGA_WIDTH];
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_Color textColor;
    SDL_Color bgColor;
    int cursor_x;
    int cursor_y;
} VGA;

int vga_init(VGA* vga);
void vga_update(VGA* vga);
void vga_write_memory(VGA* vga, uint32_t address, uint8_t value);
void vga_cleanup(VGA* vga);
void vga_scroll_up(VGA* vga);

#endif // VGA_H