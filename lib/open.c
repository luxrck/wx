#include <lib/libc.h>

syscall2(int, open, const char*, path, int, flags)
syscall3(int, mknod, const char*, path, mode_t, mode, dev_t, dev)
syscall3(off_t, lseek, int, fd, off_t, offset, int, whence)