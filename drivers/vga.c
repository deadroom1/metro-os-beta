#include "vga.h"

static uint16_t *const VGA_MEM = (uint16_t *)0xB8000;
static int cursor_x = 0, cursor_y = 0;
static uint8_t current_color = 0;

static inline uint16_t make_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static inline uint8_t make_color(vga_color_t fg, vga_color_t bg) {
    return fg | (bg << 4);
}

void vga_init(void) {
    current_color = make_color(COLOR_LGRAY, COLOR_BLACK);
    vga_clear();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_MEM[i] = make_entry(' ', current_color);
    cursor_x = cursor_y = 0;
    vga_set_cursor(0, 0);
}

void vga_set_color(vga_color_t fg, vga_color_t bg) {
    current_color = make_color(fg, bg);
}

void vga_set_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    __asm__ volatile (
        "outb %0, %1" :: "a"((uint8_t)0x0F), "Nd"((uint16_t)0x3D4));
    __asm__ volatile (
        "outb %0, %1" :: "a"((uint8_t)(pos & 0xFF)), "Nd"((uint16_t)0x3D5));
    __asm__ volatile (
        "outb %0, %1" :: "a"((uint8_t)0x0E), "Nd"((uint16_t)0x3D4));
    __asm__ volatile (
        "outb %0, %1" :: "a"((uint8_t)((pos >> 8) & 0xFF)), "Nd"((uint16_t)0x3D5));
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            VGA_MEM[cursor_y * VGA_WIDTH + cursor_x] = make_entry(' ', current_color);
        }
    } else {
        VGA_MEM[cursor_y * VGA_WIDTH + cursor_x] = make_entry(c, current_color);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) { cursor_x = 0; cursor_y++; }
    if (cursor_y >= VGA_HEIGHT) {
        /* scroll */
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++)
            VGA_MEM[i] = VGA_MEM[i + VGA_WIDTH];
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++)
            VGA_MEM[i] = make_entry(' ', current_color);
        cursor_y = VGA_HEIGHT - 1;
    }
    vga_set_cursor(cursor_x, cursor_y);
}

void vga_puts(const char *str) {
    while (*str) vga_putchar(*str++);
}

void vga_puts_at(int x, int y, const char *str) {
    int ox = cursor_x, oy = cursor_y;
    cursor_x = x; cursor_y = y;
    vga_puts(str);
    cursor_x = ox; cursor_y = oy;
    vga_set_cursor(ox, oy);
}

void vga_print_centered(int y, const char *str) {
    int len = 0;
    while (str[len]) len++;
    int x = (VGA_WIDTH - len) / 2;
    if (x < 0) x = 0;
    vga_puts_at(x, y, str);
}
