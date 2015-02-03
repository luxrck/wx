#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <kernel/kernel.h>

#include <kernel/mm.h>
#include <kernel/proc.h>
#include <kernel/trap.h>
#include <kernel/kclock.h>
#include <kernel/console.h>

static void sys_a(void)
{
	putchar('A');
}
static void sys_b(void)
{
	putchar('B');
}

static void sys_hello(void)
{
	printf("this is a syscall.\n");
}

static pid_t sys_fork(void)
{
	return fork();
}

static pid_t sys_waitpid(pid_t pid)
{
	return waitpid(pid);
}

static void sys_exit(void)
{
	exit();
}

static pid_t sys_getpid(void)
{
	return ctask->pid;
}

static pid_t sys_getppid(void)
{
	return ctask->parent->pid;
}

static time_t sys_time(time_t *store)
{
	return time(store);
}

int32_t syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	switch (syscallno) {
	case SYS_a:
		sys_a();
		break;
	case SYS_b:
		sys_b();
		break;
	case SYS_hello:
		sys_hello();
		break;
	case SYS_fork:
		return sys_fork();
	case SYS_waitpid:
		return sys_waitpid(a1);
	case SYS_exit:
		sys_exit();
		break;
	case SYS_getpid:
		return sys_getpid();
	case SYS_getppid:
		return sys_getppid();
	case SYS_time:
		return sys_time((time_t*)a1);
	default:
		return -E_INVAL;
	}

	return 0;
}

