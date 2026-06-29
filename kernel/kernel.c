#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../errors/rsod.h"
#include "../commands/sys_cmd.h"
#include "../lib/types.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002
#define MIN_RAM_MB 128
#define MAX_RAM_MB 256
#define RAM_TOLERANCE_KB 2048

typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} multiboot_mmap_entry_t;

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} multiboot_info_t;

static void sleep(uint32_t ms) {
    /* ~1ms per iteration at ~10MHz effective loop on QEMU */
    volatile uint32_t i;
    for (i = 0; i < ms * 5000; i++);
}

static size_t kstrlen(const char *s) {
    size_t n = 0; while (s[n]) n++; return n;
}

static int kstrcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a - *b;
}

static int kstrncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && *a == *b) { a++; b++; }
    return n == (size_t)-1 ? 0 : *a - *b;
}

static int cpu_supports_cpuid(void) {
    uint32_t original, modified;
    __asm__ volatile (
        "pushfl\n\t"
        "popl %0\n\t"
        "movl %0, %1\n\t"
        "xorl $0x200000, %1\n\t"
        "pushl %1\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "popl %1\n\t"
        : "=&r"(original), "=&r"(modified)
    );
    return ((original ^ modified) & 0x200000) != 0;
}

static int cpu_vendor_supported(void) {
    char vendor[13] = {0};
    uint32_t ebx, ecx, edx;
    __asm__ volatile (
        "movl $0, %%eax\n\t"
        "cpuid\n\t"
        : "=b"(ebx), "=c"(ecx), "=d"(edx)
        :
        : "eax"
    );
    for (int i = 0; i < 4; i++) vendor[i]     = (ebx >> (i*8)) & 0xFF;
    for (int i = 0; i < 4; i++) vendor[i+4]   = (edx >> (i*8)) & 0xFF;
    for (int i = 0; i < 4; i++) vendor[i+8]   = (ecx >> (i*8)) & 0xFF;
    vendor[12] = '\0';

    return kstrcmp(vendor, "GenuineIntel") == 0 ||
           kstrcmp(vendor, "AuthenticAMD") == 0;
}

static uint32_t parse_mmap_total_kb(multiboot_info_t *info) {
    if (!(info->flags & 0x40)) return 0;

    uint32_t total_kb = 0;
    uint8_t *addr = (uint8_t *)(unsigned long)info->mmap_addr;
    uint8_t *end = addr + info->mmap_length;
    while (addr < end) {
        multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t *)addr;
        if (entry->type == 1) {
            uint64_t len_kb = entry->len / 1024;
            total_kb += (uint32_t)len_kb;
        }
        addr += entry->size + sizeof(entry->size);
    }
    return total_kb;
}

static uint32_t get_total_memory_kb(void *mboot) {
    multiboot_info_t *info = (multiboot_info_t *)mboot;
    uint32_t lower_upper_kb = 0;
    uint32_t mmap_kb = parse_mmap_total_kb(info);

    if (info->flags & 0x1) {
        lower_upper_kb = info->mem_lower + info->mem_upper;
    }

    if (mmap_kb != 0) return mmap_kb;
    if (lower_upper_kb != 0) return lower_upper_kb;

    rsod_fatal("Multiboot memory info unavailable");
    return 0;
}

static void check_memory_limit(void *mboot) {
    uint32_t total_kb = get_total_memory_kb(mboot);
    uint32_t min_kb = MIN_RAM_MB * 1024u - RAM_TOLERANCE_KB;
    uint32_t max_kb = MAX_RAM_MB * 1024u;
    if (total_kb < min_kb) {
        rsod_fatal_code("RAM size too low", total_kb);
    }
    if (total_kb > max_kb) {
        rsod_fatal_code("RAM size too high", total_kb);
    }
}

/* ── boot splash ─────────────────────────────────────── */
/*  ASCII art DEADROOM centered on 80x25 VGA              */
static const char *logo[] = {
    "mmmm   mmmmmm   mm   mmmm   mmmmm   mmmm   mmmm  m    m",
    "#   \"m #        ##   #   \"m #   \"# m\"  \"m m\"  \"m ##  ##",
    "#    # #mmmmm  #  #  #    # #mmmm\" #    # #    # # ## #",
    "#    # #       #mm#  #    # #   \"m #    # #    # # \"\" #",
    "#mmm\"  #mmmmm #    # #mmm\"  #    \"  #mm#   #mm#  #    #",
    NULL
};

static void draw_splash(void) {
    vga_clear();
    vga_set_color(COLOR_LGREEN, COLOR_BLACK);

    int logo_lines = 0;
    while (logo[logo_lines]) logo_lines++;

    int start_y = (VGA_HEIGHT - logo_lines - 2) / 2;

    for (int i = 0; logo[i]; i++) {
        int len = (int)kstrlen(logo[i]);
        int x = (VGA_WIDTH - len) / 2;
        if (x < 0) x = 0;
        vga_puts_at(x, start_y + i, logo[i]);
    }

    vga_set_color(COLOR_DGRAY, COLOR_BLACK);
    const char *sub = "METRO-OS  v0.1 Beta  BUILD: D34DR00M-1.5";
    int sx = (VGA_WIDTH - (int)kstrlen(sub)) / 2;
    vga_puts_at(sx, start_y + logo_lines + 1, sub);

    /* keep the splash visible for 50 seconds */
    vga_set_color(COLOR_LGREEN, COLOR_BLACK);
    sleep(50000);
}

/* ── shell ───────────────────────────────────────────── */
#define CMD_BUF 128

static void shell_prompt(void) {
    const char *cwd = cmd_get_current_dir();
    vga_set_color(COLOR_LGREEN, COLOR_BLACK);
    vga_puts("METRO");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
    vga_puts(":");
    vga_set_color(COLOR_LCYAN, COLOR_BLACK);
    vga_puts(cwd);
    vga_set_color(COLOR_LGREEN, COLOR_BLACK);
    vga_puts("> ");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
}

static void shell_header(void) {
    vga_clear();
    vga_set_color(COLOR_LMAGENTA, COLOR_BLACK);
    vga_puts("===============================================================\n");
    vga_puts("      METRO-OS 0.1 Beta  |  Live Shell\n");
    vga_puts("===============================================================\n");
    vga_set_color(COLOR_YELLOW, COLOR_BLACK);
    vga_puts("Welcome to METRO-OS. Type 'help' for commands.\n");
    vga_puts("Use 'clear' or 'cls' to refresh the screen.\n\n");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
}

static void shell_help(void) {
    vga_set_color(COLOR_YELLOW, COLOR_BLACK);
    vga_puts("Available commands:\n");
    vga_puts("  help            - show this help menu\n");
    vga_puts("  cls / clear     - clear the screen\n");
    vga_puts("  dir / ls        - list available entries\n");
    vga_puts("  cd <dir>        - change directory\n");
    vga_puts("  pwd             - show current directory\n");
    vga_puts("  cat <file>      - show file contents\n");
    vga_puts("  ver             - show OS version\n");
    vga_puts("  mem             - show memory information\n");
    vga_puts("  time / date     - show current RTC time\n");
    vga_puts("  cpuinfo         - show CPU vendor information\n");
    vga_puts("  echo <text>     - print text\n");
    vga_puts("  copy <text>     - print the given text\n");
    vga_puts("  touch <file>    - create a dummy file\n");
    vga_puts("  mkdir <dir>     - create a directory\n");
    vga_puts("  rmdir <dir>     - remove a directory\n");
    vga_puts("  rm <file>       - remove a file\n");
    vga_puts("  uptime          - show system uptime\n");
    vga_puts("  countdown <n>   - print countdown from n\n");
    vga_puts("  banner <text>   - print big banner text\n");
    vga_puts("  restart         - reboot the system\n");
    vga_puts("  shutdown        - halt the system\n");
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
}

static void run_command(char *buf) {
    /* strip trailing spaces */
    int len = (int)kstrlen(buf);
    while (len > 0 && buf[len-1] == ' ') buf[--len] = 0;
    if (len == 0) return;

    if (kstrcmp(buf, "help") == 0)        shell_help();
    else if (kstrcmp(buf, "cls") == 0 || kstrcmp(buf, "clear") == 0) shell_header();
    else if (kstrcmp(buf, "mem") == 0)    cmd_mem();
    else if (kstrcmp(buf, "time") == 0 || kstrcmp(buf, "date") == 0) cmd_time();
    else if (kstrcmp(buf, "uptime") == 0) cmd_uptime();
    else if (kstrcmp(buf, "cpuinfo") == 0) cmd_cpuinfo();
    else if (kstrcmp(buf, "shutdown") == 0) cmd_shutdown();
    else if (kstrcmp(buf, "dir") == 0 || kstrcmp(buf, "ls") == 0) cmd_dir();
    else if (kstrncmp(buf, "cd ", 3) == 0) cmd_cd(buf + 3);
    else if (kstrcmp(buf, "pwd") == 0 || kstrcmp(buf, "cmdlocate") == 0) cmd_locate();
    else if (kstrncmp(buf, "cat ", 4) == 0) cmd_cat(buf + 4);
    else if (kstrncmp(buf, "echo ", 5) == 0) cmd_echo(buf + 5);
    else if (kstrncmp(buf, "touch ", 6) == 0) cmd_touch(buf + 6);
    else if (kstrncmp(buf, "mkdir ", 6) == 0) cmd_mkdir(buf + 6);
    else if (kstrncmp(buf, "rmdir ", 6) == 0) cmd_rmdir(buf + 6);
    else if (kstrncmp(buf, "rm ", 3) == 0) cmd_rm(buf + 3);
    else if (kstrncmp(buf, "countdown ", 10) == 0) cmd_countdown(buf + 10);
    else if (kstrncmp(buf, "banner ", 7) == 0) cmd_banner(buf + 7);
    else if (kstrcmp(buf, "restart") == 0) cmd_restart();
    else if (kstrcmp(buf, "ver") == 0) {
        vga_set_color(COLOR_LCYAN, COLOR_BLACK);
        vga_puts("METRO-OS v0.1 Beta BUILD D34DR00M-1.5\n");
        vga_set_color(COLOR_WHITE, COLOR_BLACK);
    }
    else if (kstrncmp(buf, "copy ", 5) == 0) cmd_copy(buf + 5);
    else {
        vga_set_color(COLOR_LRED, COLOR_BLACK);
        vga_puts("Unknown command: ");
        vga_puts(buf);
        vga_putchar('\n');
        vga_set_color(COLOR_WHITE, COLOR_BLACK);
    }
}

static void shell_run(void) {
    shell_header();

    char buf[CMD_BUF];
    int pos = 0;

    shell_prompt();

    while (1) {
        char c = keyboard_getchar();
        if (!c) continue;

        if (c == '\n') {
            vga_putchar('\n');
            buf[pos] = 0;
            run_command(buf);
            pos = 0;
            shell_prompt();
        } else if (c == '\b') {
            if (pos > 0) { pos--; vga_putchar('\b'); }
        } else if (pos < CMD_BUF - 1) {
            buf[pos++] = c;
            vga_putchar(c);
        }
    }
}

/* ── entry ───────────────────────────────────────────── */
void kernel_main(uint32_t magic, void *mboot) {
    vga_init();
    keyboard_init();
    draw_splash();

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        rsod_fatal_code("Invalid boot signature", magic);
    }
    if (mboot == NULL) {
        rsod_fatal("Missing Multiboot information");
    }
    if (!cpu_supports_cpuid()) {
        rsod_fatal("Unsupported CPU: CPUID unavailable");
    }
    if (!cpu_vendor_supported()) {
        rsod_fatal_code("Unsupported CPU vendor", 0xC001);
    }
    check_memory_limit(mboot);

    shell_run();
}
