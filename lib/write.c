#include <lib/libc.h>

syscall3(int, write, int, fd, const char*, buf, size_t, count)

void sync(void)
{
	asm volatile("int %0"
		:
		: "i"(T_SYSCALL),
		  "a"(SYS_sync)
		: "cc", "memory");
}