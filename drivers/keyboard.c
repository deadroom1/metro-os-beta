#include "keyboard.h"

#define KB_DATA    0x60
#define KB_STATUS  0x64

static int shift_pressed = 0;

static const char scancode_map[] = {
    0,   0,  '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' '
};

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void keyboard_init(void) {
    shift_pressed = 0;
}

char keyboard_getchar(void) {
    while (1) {
        while (!(inb(KB_STATUS) & 1));
        uint8_t sc = inb(KB_DATA);

        if (sc == 0x2A || sc == 0x36) {
            shift_pressed = 1;
            continue;
        }
        if (sc == 0xAA || sc == 0xB6) {
            shift_pressed = 0;
            continue;
        }
        if (sc & 0x80) continue;

        if (sc < sizeof(scancode_map)) {
            char c = scancode_map[sc];
            if (shift_pressed) {
                if (c >= 'a' && c <= 'z') {
                    c = (char)(c - 'a' + 'A');
                } else {
                    switch (c) {
                        case '1': c = '!'; break;
                        case '2': c = '@'; break;
                        case '3': c = '#'; break;
                        case '4': c = '$'; break;
                        case '5': c = '%'; break;
                        case '6': c = '^'; break;
                        case '7': c = '&'; break;
                        case '8': c = '*'; break;
                        case '9': c = '('; break;
                        case '0': c = ')'; break;
                        case '-': c = '_'; break;
                        case '=': c = '+'; break;
                        case '[': c = '{'; break;
                        case ']': c = '}'; break;
                        case ';': c = ':'; break;
                        case '\'': c = '"'; break;
                        case ',': c = '<'; break;
                        case '.': c = '>'; break;
                        case '/': c = '?'; break;
                        case '`': c = '~'; break;
                        case '\\': c = '|'; break;
                        default: break;
                    }
                }
            }
            return c;
        }
        return 0;
    }
}
