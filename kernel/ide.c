#include <x86.h>
#include <assert.h>

#include <kernel/fs.h>
#include <kernel/device.h>

// We assume that we always on IDE0

// http://wiki.osdev.org/ATA_PIO_Mode
#define IDE_BSY		0x80
#define IDE_RDY		0x40
#define IDE_DF		0x20
#define IDE_ERR		0x01

#define IDEC_READ	0x20
#define IDEC_WRITE	0x30

// no error check.
static int idewait(void)
{
	int r = 0;
	while (((r = inb(0x1F7)) & (IDE_BSY|IDE_RDY)) != IDE_RDY);
	if ((r & (IDE_DF|IDE_ERR)))
		return -1;
	return 0;
}

static void iderw(int cmd, char *buf, dev_t dev, uint32_t secno, uint32_t count)
{
	idewait();

	// 0 means 256 sectors(128KB)
	outb(0x1F2, 2);
	outb(0x1F3, secno & 0xFF);
	outb(0x1F4, (secno >> 8) & 0xFF);
	outb(0x1F5, (secno >> 16) & 0xFF);
	outb(0x1F6, 0xE0 | (minor(dev)<<4) | ((secno>>24)&0x0F));
	outb(0x1F7, cmd);
	
	for (int i=0; i<count; i++) {
		idewait();
		if (cmd == IDEC_READ)
			insl(0x1F0, buf + i*SECTSIZE, SECTSIZE/4);
		else if (cmd == IDEC_WRITE)
			outsl(0x1F0, buf + i*SECTSIZE, SECTSIZE/4);
		else
			panic("iderw: unknown cmd for ide driver.");
	}
}

static void ide_request(int rw, char *buf, dev_t dev, uint32_t secno, uint32_t count)
{
	int cmd = rw ? IDEC_WRITE : IDEC_READ;
	iderw(cmd, buf, dev, secno, count);
}

static struct dev ide_dev;

void ideinit(void)
{
	ide_dev.dev = MKDEV(DEV_HD, 0);
	ide_dev.fops = NULL;
	ide_dev.request = ide_request;

	devregister(&ide_dev);
}