#include <x86.h>
#include <string.h>
//#include <assert.h>

#include <kernel/kernel.h>
#include <kernel/console.h>

// Text-mode CGA display output.
struct cga_display {
    uint16_t bport;     // base port
    uint16_t *buf;      // display buffer
    uint16_t cpos;      // cursor position
} cga;

static void cga_init(void)
{
    cga.buf = (uint16_t*)(KERNBASE + 0xB8000);
    cga.bport = 0x3D4;

    // Get cursor location.
    outb(cga.bport, 14);
    cga.cpos = inb(cga.bport + 1) << 8;
    outb(cga.bport, 15);
    cga.cpos |= inb(cga.bport + 1);
}

static void cga_putchar(int c)
{
    // We just use black on white
    //if (!(c & ~0xFF)) c |= 0x0700;

    switch (c) {
    case '\b':
        if (cga.cpos > 0) {
            cga.cpos--;
            cga.buf[cga.cpos] = 0x0700 | ' ';
        }
        break;
    case '\n':
        cga.cpos += 80;
    case '\r':
        cga.cpos -= (cga.cpos % 80);
        break;
    case '\t':
        for (int i=0; i<TAB_SIZE - (cga.cpos+1) % 80 % TAB_SIZE; i++)
            cga_putchar(' ');
        break;
    default:
        cga.buf[cga.cpos++] = 0x0700 | c;
        break;
    }

    // If text overflow, scroll down screen.
    if (cga.cpos >= 80*25) {
        memcpy(cga.buf, cga.buf + 80, 80 * 24 * sizeof(uint16_t));
        for (int i = 0; i < 80; i++)
            cga.buf[80*24+i] = 0x0700 | ' ';
        cga.cpos -= 80;
    }

    // Update cursor location.
    outb(cga.bport, 14);
    outb(cga.bport + 1, cga.cpos >> 8);
    outb(cga.bport, 15);
    outb(cga.bport + 1, cga.cpos);
}

// PS/2 Keyboard input.
// We use PS/2 Keyboard Scan Code Set I, because i8042 supplies this set codes by default.
// I simplify this set so all keys only have one-byte-length make codes and break codes.
// We don't use interrupt method to use keyboard.
#define KEY_DEL 0XE9
static uint8_t normalmap[256] =
{
    00,   033,  '1',  '2',  '3',  '4',  '5',  '6',  // 0x00
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10
    'o',  'p',  '[',  ']',  '\n', 00,   'a',  's',
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20
    '\'', '`',  00,   '\\', 'z',  'x',  'c',  'v',
    'b',  'n',  'm',  ',',  '.',  '/',  00,   '*',  // 0x30
    00,   ' ',
    [0xD3] = KEY_DEL
};

static uint8_t shiftmap[256] =
{
    00,   033,  '!',  '@',  '#',  '$',  '%',  '^',  // 0x00
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10
    'O',  'P',  '{',  '}',  '\n', 00,   'A',  'S',
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20
    '"',  '~',  00,   '|',  'Z',  'X',  'C',  'V',
    'B',  'N',  'M',  '<',  '>',  '?',  00,   '*',  // 0x30
    00,   ' ',
    [0xD3] = KEY_DEL
};

static int kbd_getchar(void)
{
	static bool shift = 0;
	static bool capslock = 0;
	// 0x01: KBS_OBF
	while (!(inb(0x64) & 0x01));
	uint8_t data = inb(0x60);
	uint8_t key = 0;
	if (data == 0x2A) shift = 1;
	else if (data == 0x3A) capslock = !capslock;
	else if (data == 0xAA) shift = 0;
	else key = shift ? shiftmap[data] : normalmap[data];
	if (capslock && key >= 'a' && key <= 'z') key = shiftmap[data];
	return key;
}

void consinit(void)
{
	cga_init();
}

int cons_getchar(void)
{
	return kbd_getchar();
}

// High-level console I/O.
void putchar(int c)
{
    cga_putchar(c);
}

int getchar(void)
{
    return kbd_getchar();
}

int iscons(int fdnum)
{
    // used by readline
    return 1;
}
