#include <stdio.h>

#include <lib/libc.h>

syscall2(int, fstat, int, fd, struct stat*, st)
syscall2(int, stat, const char*, path, struct stat*, st)