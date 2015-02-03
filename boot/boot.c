#include <x86.h>
#include <elf.h>

/**********************************************************************
 * This a dirt simple boot loader, whose sole job is to boot
 * an ELF kernel image from the first IDE hard disk.
 *
 * DISK LAYOUT
 *  * This program(boot.S and main.c) is the bootloader.  It should
 *    be stored in the first sector of the disk.
 *
 *  * The 2nd sector onward holds the kernel image.
 *
 *  * The kernel image must be in ELF format.
 *
 * BOOT UP STEPS
 *  * when the CPU boots it loads the BIOS into memory and executes it
 *
 *  * the BIOS intializes devices, sets of the interrupt routines, and
 *    reads the first sector of the boot device(e.g., hard-drive)
 *    into memory and jumps to it.
 *
 *  * Assuming this boot loader is stored in the first sector of the
 *    hard-drive, this code takes over...
 *
 *  * control starts in boot.S -- which sets up protected mode,
 *    and a stack so C code then run, then calls bootmain()
 *
 *  * bootloader() in this file takes over, reads in the kernel and jumps to it.
 **********************************************************************/

#define SECTSIZE	512
#define kernel		((struct elf *) 0x10000) // scratch space

void readsect(void *dst, uint32_t base, uint32_t nsec);

void bootloader(void)
{
	struct proghdr *ph, *eph;

	// read 1st page off disk
	readsect((void*)kernel, 1, 8);

	// is this a valid ELF?
	if (kernel->magic != ELF_MAGIC) return;

	// load each program segment (ignores ph flags)
	ph = (struct proghdr *) ((uint8_t *) kernel + kernel->phoff);
	eph = ph + kernel->phnum;
	for (; ph < eph; ph++)
		// 1st sector is used by bootloader, kernel start at 2nd sector.
		readsect((void*)ph->pa, (ph->offset/SECTSIZE)+1,
			(ph->pa+ph->memsz)/SECTSIZE - ph->pa/SECTSIZE);

	// call the entry point from the ELF header
	// note: does not return!
	((void (*)(void)) (kernel->entry))();
}

void waitdisk(void)
{
	// wait for disk reaady
	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;
}

void readsect(void *dst, uint32_t sector, uint32_t nsec)
{
	// wait for disk to be ready
	waitdisk();

	outb(0x1F2, nsec);
	outb(0x1F3, sector);
	outb(0x1F4, sector >> 8);
	outb(0x1F5, sector >> 16);
	outb(0x1F6, (sector >> 24) | 0xE0);
	outb(0x1F7, 0x20);	// cmd 0x20 - read sectors

	// wait for disk to be ready
	for (; nsec>0; nsec--, dst+=SECTSIZE) {
		waitdisk();
		// read a sector
		insl(0x1F0, dst, SECTSIZE/4);
	}

}
