#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* ANSI-style colors */
typedef enum {
    COLOR_BLACK   = 0, COLOR_BLUE      = 1, COLOR_GREEN  = 2,
    COLOR_CYAN    = 3, COLOR_RED       = 4, COLOR_MAGENTA= 5,
    COLOR_BROWN   = 6, COLOR_LGRAY     = 7, COLOR_DGRAY  = 8,
    COLOR_LBLUE   = 9, COLOR_LGREEN    = 10,COLOR_LCYAN  = 11,
    COLOR_LRED    = 12,COLOR_LMAGENTA  = 13,COLOR_YELLOW = 14,
    COLOR_WHITE   = 15
} vga_color_t;

void vga_init(void);
void vga_clear(void);
void vga_set_color(vga_color_t fg, vga_color_t bg);
void vga_putchar(char c);
void vga_puts(const char *str);
void vga_puts_at(int x, int y, const char *str);
void vga_set_cursor(int x, int y);
void vga_print_centered(int y, const char *str);

#endif
