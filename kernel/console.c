#include <x86.h>
#include <assert.h>
#include <string.h>

#include <kernel/fs.h>
#include <kernel/proc.h>
#include <kernel/trap.h>
#include <kernel/device.h>

// Serial Port.
#define COM1    0x3F8
static void serial_init(void)
{
	outb(COM1 + 1, 0x00);    // Disable all interrupts
	outb(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outb(COM1 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outb(COM1 + 1, 0x00);    //                  (hi byte)
	outb(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
	outb(COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

static int serial_getchar(void)
{
	while (inb(COM1 + 5) & 1);
	return inb(COM1);
}

static void serial_putchar(int c)
{
	while (!(inb(COM1 + 5) & 0x20));
	outb(COM1, c);
}

// Text-mode CGA display output.
#define TAB_SIZE    8
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

	if (!c) return;

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
#define KEY_DEL 0xE9
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

#define IBUFLEN     2000
struct input_buf {
	char data[IBUFLEN];
	uint16_t rpos;
	uint16_t wpos;
} ibuf;

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

void kbd_intr(void)
{
	char key = kbd_getchar();
	if (key == '\b' || key == KEY_DEL) {
		if (ibuf.wpos != ibuf.rpos) {
			putchar(key);
			ibuf.wpos--;
			return;
		}
	} else {
		putchar(key);
		if (key) ibuf.data[ibuf.wpos++] = key;
		if (ibuf.wpos == IBUFLEN) ibuf.wpos = 0;
		if (key == '\n') wakeup(&ibuf.rpos);
	}
}

/*
 * Console Device Init
 */

static int cons_read(struct file *file, char *buf, size_t count, off_t off)
{
	int rc = 0;

	if (ibuf.rpos == ibuf.wpos)
		sleep(&ibuf.rpos);

	while (rc < count) {
		buf[rc] = getchar();
		if (buf[rc] == '\n' || buf[rc] == '\0')
			break;
		rc++;
	}

	return rc;
}

static int cons_write(struct file *file, const char *buf, size_t count, off_t off)
{
	for (int i=0; i<count; i++)
		putchar(buf[i]);
	return count;
}

static const struct fileops cons_fops = {
	.open = NULL,
	.read = cons_read,
	.write = cons_write,
	.release = NULL
};

static struct dev cons_dev;

void consinit(void)
{
	cga_init();
	serial_init();

	cons_dev.dev = MKDEV(DEV_TTYC, 0);
	cons_dev.fops = &cons_fops;

	devregister(&cons_dev);
}

// High-level console I/O.

void putchar(int c)
{
	cga_putchar(c);
	serial_putchar(c);
}

int getchar(void)
{
	if (ibuf.rpos == IBUFLEN) ibuf.rpos = 0;

	if (ibuf.rpos == ibuf.wpos)
		return 0;

	return ibuf.data[ibuf.rpos++];

	//while ((buf[rc++] = ibuf.data[ibuf.rpos++]) != '\n' && ibuf.rpos != ibuf.wpos)
	//	if (ibuf.rpos == IBUFLEN) ibuf.rpos = 0;
}

int iscons(int fdnum)
{
	// used by readline
	return 1;
}
