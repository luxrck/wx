#include <lib/libc.h>

syscall0(pid_t, fork)
syscall2(int, execv, const char *, path, char **, argv)
syscall0(pid_t, getpid)
syscall0(pid_t, getppid)