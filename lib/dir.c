#include <stdio.h>

#include <lib/libc.h>

syscall1(int, mkdir, const char*, path)
syscall1(int, rmdir, const char*, path)

syscall1(int, fchdir, int, fd)
syscall1(int, chdir, const char*, path)

syscall2(int, readdir, int, fd, struct dir*, dir)