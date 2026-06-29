#include "../drivers/vga.h"

void cmd_copy(const char *args) {
    if (!args || !*args) {
        vga_puts("Usage: copy <text>\n");
        return;
    }
    vga_set_color(COLOR_YELLOW, COLOR_BLACK);
    vga_puts("Copied: ");
    vga_puts(args);
    vga_putchar('\n');
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
}
