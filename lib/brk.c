#include <lib/libc.h>

syscall1(int, brk, void*, addr)
syscall1(void*, sbrk, int, inc)