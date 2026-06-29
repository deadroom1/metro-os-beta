#include "../drivers/vga.h"
#include "../drivers/serial.h"
#include "../errors/rsod.h"
#include "../lib/types.h"

static void rsod_append_hex(char *dst, uint32_t value) {
    const char hex[] = "0123456789ABCDEF";
    dst[0] = '0';
    dst[1] = 'x';
    for (int i = 9; i >= 2; i--) {
        dst[i] = hex[value & 0xF];
        value >>= 4;
    }
    dst[10] = '\0';
}

static void rsod_build_line(char *dst, const char *message, uint32_t code, int has_code) {
    const char *prefix = "LOG: ";
    int pos = 0;
    while (prefix[pos] && pos < 79) {
        dst[pos] = prefix[pos];
        pos++;
    }

    int i = 0;
    while (message[i] && pos < 79) {
        dst[pos++] = message[i++];
    }

    if (has_code && pos < 76) {
        dst[pos++] = ' ';
        dst[pos++] = '(';
        rsod_append_hex(dst + pos, code);
        pos += 10;
        dst[pos++] = ')';
    }
    dst[pos] = '\0';
}

static void rsod_show_screen(const char *message, uint32_t code, int has_code) {
    char log_line[96];
    rsod_build_line(log_line, message, code, has_code);

    serial_init();
    vga_set_color(COLOR_WHITE, COLOR_RED);
    vga_clear();
    vga_print_centered(10, "METRO-OS 0.1 Beta  RED SCREEN OF DEATH");
    vga_print_centered(12, log_line);
    if (has_code) {
        vga_print_centered(14, "Please reboot or check hardware.");
    } else {
        vga_print_centered(14, "Please reboot or check hardware.");
    }

    serial_puts("RSOD: ");
    serial_puts(log_line);
    serial_puts("\n");
}

void rsod_fatal(const char *message) {
    vga_init();
    rsod_show_screen(message, 0, 0);
    while (1) __asm__ volatile ("hlt");
}

void rsod_fatal_code(const char *message, uint32_t code) {
    vga_init();
    rsod_show_screen(message, code, 1);
    while (1) __asm__ volatile ("hlt");
}
