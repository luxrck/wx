#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <x86.h>

#include <kernel/mm.h>
#include <kernel/pic.h>
#include <kernel/proc.h>
#include <kernel/console.h>
#include <kernel/kshell.h>
#include <kernel/trap.h>

void i386_init(void)
{
	extern char edata[], end[];

	// Before doing anything else, complete the ELF loading process.
	// Clear the uninitialized global data (BSS) section of our program.
	// This ensures that all static/global variables start out zero.
	memset(edata, 0, end - edata);
	consinit();
	printf("6828 decimal is %o octal!\n", 6828);
	
	meminit();
	procinit();
	trapinit();
	
	picinit();

	ENV_CREATE(user__hello);
	scheduler();
}

/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	if (panicstr)
		goto dead;
	panicstr = fmt;

	// Be extra sure that the machine is in as reasonable state
	__asm __volatile("cli; cld");

	va_start(ap, fmt);
	printf("kernel panic on CPU 0 at %s:%d: ", file, line);
	vprintf(fmt, ap);
	putchar('\n');
	va_end(ap);

dead:
	/* break into the kernel monitor */
	while (1)
		kshell();
}

/* like panic, but don't */
void _warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	printf("kernel warning at %s:%d: ", file, line);
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
}
