#include <stdio.h>
#include <assert.h>
#include <lib/libc.h>

#include <kernel/kernel.h>

inline int32_t
syscall(uint32_t num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret;

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.

	asm volatile("int %1\n"
		: "=a" (ret)
		: "i" (T_SYSCALL),
		  "a" (num),
		  "b" (a1),
		  "c" (a2),
		  "d" (a3),
		  "S" (a4),
		  "D" (a5)
		: "cc", "memory");

	//if(check && ret > 0)
	//	panic("syscall %d returned %d (> 0)", num, ret);

	return ret;
}

void sys_a(void)
{
	syscall(SYS_a, 0, 0, 0, 0, 0);
}

void sys_b(void)
{
	syscall(SYS_b, 0, 0, 0, 0, 0);
}

void sys_hello(void)
{
	syscall(SYS_hello, 0, 0, 0, 0, 0);
}

pid_t sys_fork(void)
{
	return syscall(SYS_fork, 0, 0, 0, 0, 0);
}

pid_t sys_waitpid(pid_t pid)
{
	return syscall(SYS_waitpid, pid, 0, 0, 0, 0);
}

void sys_exit(void)
{
	syscall(SYS_exit, 0, 0, 0, 0, 0);
}

pid_t sys_getpid(void)
{
	return syscall(SYS_getpid, 0, 0, 0, 0, 0);
}

pid_t sys_getppid(void)
{
	return syscall(SYS_getppid, 0, 0, 0, 0, 0);
}

time_t sys_time(time_t *t)
{
	return syscall(SYS_time, (uint32_t)t, 0, 0, 0, 0);
}