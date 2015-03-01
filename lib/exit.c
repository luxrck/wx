#include <lib/libc.h>

void exit(void)
{
	asm volatile("int %0"
		:
		: "i"(T_SYSCALL),
		  "a"(SYS_exit)
		: "cc", "memory");
}