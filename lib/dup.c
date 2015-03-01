#include <lib/libc.h>

syscall1(int, dup, int, ofd)
syscall2(int, dup2, int, ofd, int, nfd)