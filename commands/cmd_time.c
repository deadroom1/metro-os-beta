#include "../drivers/vga.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t v; __asm__ volatile ("inb %1,%0":"=a"(v):"Nd"(port)); return v;
}
static inline void outb(uint16_t port, uint8_t v) {
    __asm__ volatile ("outb %0,%1"::"a"(v),"Nd"(port));
}

static uint8_t rtc_read(uint8_t reg) {
    outb(0x70, reg); return inb(0x71);
}
static uint8_t bcd(uint8_t v) { return (v >> 4) * 10 + (v & 0xF); }

static void print2(uint8_t n) {
    vga_putchar('0' + n / 10);
    vga_putchar('0' + n % 10);
}

void cmd_time(void) {
    uint8_t h = bcd(rtc_read(0x04));
    uint8_t m = bcd(rtc_read(0x02));
    uint8_t s = bcd(rtc_read(0x00));
    vga_set_color(COLOR_LCYAN, COLOR_BLACK);
    vga_puts("Time: ");
    print2(h); vga_putchar(':');
    print2(m); vga_putchar(':');
    print2(s); vga_putchar('\n');
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
}
