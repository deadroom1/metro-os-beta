#include "../drivers/vga.h"

void cmd_mem(void) {
    vga_set_color(COLOR_LGREEN, COLOR_BLACK);
    vga_puts("Memory Information:\n");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
    vga_puts("  Base Memory : 640 KB\n");
    vga_puts("  Extended    : 127 MB\n");
    vga_puts("  Total       : 128 MB\n");
}
