#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */

#ifndef EOF
#define EOF		('\00')
#endif /* !EOF */

#define stdin	0
#define stdout	1
#define stderr	2

typedef _Bool bool;
enum { false, true };

// Intergers
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

// Pointers
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;
typedef uint32_t physaddr_t;

// Page numbers are 32 bits long.
typedef uint32_t ppn_t;

typedef uint32_t size_t;
typedef int32_t ssize_t;
typedef int32_t off_t;

// processers
typedef int32_t pid_t;

// time
typedef int32_t time_t;

// fs
typedef uint16_t ino_t;
typedef uint8_t mode_t;
typedef uint8_t nlink_t;
typedef uint8_t dev_t;

// Efficient min and max operations
#define MIN(_a, _b)				\
({								\
	typeof(_a) __a = (_a);		\
	typeof(_b) __b = (_b);		\
	__a <= __b ? __a : __b;		\
})
#define MAX(_a, _b)				\
({								\
	typeof(_a) __a = (_a);		\
	typeof(_b) __b = (_b);		\
	__a >= __b ? __a : __b;		\
})

// Rounding operations (efficient when n is a power of 2)
// Round down to the nearest multiple of n
#define ROUNDDOWN(a, n)			\
({								\
	uint32_t __a = (uint32_t) (a);	\
	(typeof(a)) (__a - __a % (n));	\
})
// Round up to the nearest multiple of n
#define ROUNDUP(a, n)			\
({								\
	uint32_t __n = (uint32_t) (n);	\
	(typeof(a)) (ROUNDDOWN((uint32_t) (a) + __n - 1, __n));	\
})

#define O_RDONLY	1
#define O_WRONLY	2
#define O_RDWR		3
#define O_CREAT		4
#define O_APPEND	8

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#define S_IFREG		1
#define S_IFCHR		2
#define S_IFBLK		3
#define S_IFFIFO	4
#define S_IFPIPE	5

#define makedev(ma, mi)	((ma << 3) | (mi & 7))
#define major(dev)	(dev >> 3)
#define minor(dev)	(dev  & 7)

#define WNOHANG		1

struct dir;

struct stat {
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	dev_t st_rdev;
	off_t st_size;
};

// POSIX system calls
enum {
	SYS_setup = 0,
	SYS_exit,
	SYS_fork,
	SYS_brk,
	SYS_sbrk,
	SYS_read,
	SYS_write,
	SYS_open,
	SYS_close,
	SYS_waitpid,
	SYS_creat,
	SYS_link,
	SYS_unlink,
	SYS_getcwd,
	SYS_execv,
	SYS_chdir,
	SYS_fchdir,
	SYS_time,
	SYS_mknod,
	SYS_readdir,
	SYS_rename,
	SYS_break,
	SYS_stat,
	SYS_fstat,
	SYS_lseek,
	SYS_getpid,
	SYS_mkdir = 39,
	SYS_rmdir,
	SYS_dup,
	SYS_pipe,
	SYS_uname = 59,
	SYS_dup2 = 63,
	SYS_getppid,
	SYS_sync,
	NSYSCALLS
};

int hello(void);
pid_t fork(void);
int execv(const char *path, char **argv);
pid_t waitpid(pid_t pid, int options);
pid_t wait(void);
pid_t getpid(void);
pid_t getppid(void);
void exit(void);
int brk(void *addr);
void* sbrk(int inc);

int open(const char *path, int flags);
int close(int fd);
int read(int fd, char *buf, size_t count);
int write(int fd, const char *buf, size_t count);
int mknod(const char *path, mode_t mode, dev_t dev);
off_t lseek(int fd, off_t offset, int whence);
int dup(int ofd);
int dup2(int ofd, int nfd);
int mkdir(const char *path);
int rmdir(const char *path);
int link(const char *op, const char *np);
int rename(const char *op, const char *np);
int unlink(const char *path);
int pipe(int pfd[2]);
int fstat(int fd, struct stat *st);
int stat(const char *path, struct stat *st);
int fchdir(int fd);
int chdir(const char *path);
int readdir(int fd, struct dir *sd);
void sync(void);

void putchar(int c);
int	getchar(void);
int	printf(const char *fmt, ...);
int fprintf(int fd, const char *fmt, ...);
int vprintf(const char *fmt, va_list args);
//int sprintf(char *s, const char *fmt, ...);
//int vsprintf(char *s, const char *fmt, va_list args);
char* readline(const char *prompt);

void free(void *addr);
void* malloc(size_t size);

#endif /* !__STDIO_H__ */
