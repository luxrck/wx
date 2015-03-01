#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <kernel/kernel.h>

#include <kernel/mm.h>
#include <kernel/fs.h>
#include <kernel/proc.h>
#include <kernel/trap.h>
#include <kernel/device.h>
#include <kernel/kclock.h>

int32_t syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	switch (syscallno) {
	case SYS_fork:
		return fork();
	case SYS_execv:
		return execv((const char*)a1, (char**)a2);
	case SYS_waitpid:
		return waitpid(a1, a2);
	case SYS_exit:
		exit();
		break;
	case SYS_getpid:
		return ctask->pid;
	case SYS_getppid:
		return ctask->parent->pid;
	case SYS_brk:
		return brk((void*)a1);
	case SYS_sbrk:
		return (int32_t)sbrk(a1);
	case SYS_time:
		return time((time_t*)a1);
	case SYS_open:
		return open((const char*)a1, a2);
	case SYS_close:
		return close(a1);
	case SYS_read:
		return read(a1, (char*)a2, a3);
	case SYS_write:
		return write(a1, (const char*)a2, a3);
	case SYS_mknod:
		return mknod((const char*)a1, a2, a3);
	case SYS_lseek:
		return lseek(a1, a2, a3);
	case SYS_dup:
		return dup(a1);
	case SYS_dup2:
		return dup2(a1, a2);
	case SYS_mkdir:
		return mkdir((const char*)a1);
	case SYS_rmdir:
		return rmdir((const char*)a1);
	case SYS_link:
		return link((const char*)a1, (const char*)a2);
	case SYS_unlink:
		return unlink((const char*)a1);
	case SYS_pipe:
		return pipe((int*)a1);
	case SYS_stat:
		return stat((const char*)a1, (struct stat*)a2);
	case SYS_fstat:
		return fstat(a1, (struct stat*)a2);
	case SYS_chdir:
		return chdir((const char*)a1);
	case SYS_fchdir:
		return fchdir(a1);
	case SYS_rename:
		return rename((const char*)a1, (const char*)a2);
	case SYS_readdir:
		return readdir(a1, (struct dir*)a2);
	case SYS_sync:
		sync();
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

