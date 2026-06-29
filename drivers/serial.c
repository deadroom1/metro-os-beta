#include "serial.h"

#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static int serial_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

void serial_putchar(char c) {
    while (!serial_transmit_empty());
    outb(COM1 + 0, c);
}

void serial_puts(const char *s) {
    while (*s) serial_putchar(*s++);
}

void serial_puthex(uint32_t value) {
    const char *hex = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        serial_putchar(hex[(value >> (i * 4)) & 0xF]);
    }
}
