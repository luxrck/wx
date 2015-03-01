#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <x86.h>

#include <kernel/mm.h>
#include <kernel/fs.h>
#include <kernel/pic.h>
#include <kernel/proc.h>
#include <kernel/device.h>
#include <kernel/kshell.h>
#include <kernel/trap.h>

void i386_init(void)
{
	extern char edata[], end[];

	// Before doing anything else, complete the ELF loading process.
	// Clear the uninitialized global data (BSS) section of our program.
	// This ensures that all static/global variables start out zero.
	memset(edata, 0, end - edata);
	
	meminit();
	trapinit();
	
	picinit();

	devinit();
	fsinit();
	
	procinit();
	
	//ENV_CREATE(user_init);
	
	scheduler();
}