#include <stdio.h>

#include <lib/libc.h>

syscall2(int, link, const char*, op, const char*, np)
syscall1(int, unlink, const char*, path)

syscall2(int, rename, const char*, op, const char*, np)