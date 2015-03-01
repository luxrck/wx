#include <lib/libc.h>

syscall2(pid_t, waitpid, pid_t, pid, int, options)

pid_t wait(void)
{
	return waitpid(-1, 0);
}