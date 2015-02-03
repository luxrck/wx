#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */

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

// POSIX system calls
enum {
	SYS_setup = 0,
	SYS_a,
	SYS_b,
	SYS_hello,
	SYS_exit,
	SYS_fork,
	SYS_read,
	SYS_write,
	SYS_open,
	SYS_close,
	SYS_waitpid,
	SYS_creat,
	SYS_link,
	SYS_unlink,
	SYS_execve,
	SYS_chdir,
	SYS_time,
	SYS_mknod,
	SYS_chmod,
	SYS_chown,
	SYS_break,
	SYS_stat,
	SYS_lseek,
	SYS_getpid,
	SYS_mkdir = 39,
	SYS_rmdir,
	SYS_dup,
	SYS_pipe,
	SYS_uname = 59,
	SYS_dup2 = 63,
	SYS_getppid,
	NSYSCALLS
};

void putchar(int c);
int	getchar(void);
int	printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list args);
//int sprintf(char *s, const char *fmt, ...);
//int vsprintf(char *s, const char *fmt, va_list args);
char* readline(const char *prompt);


#endif /* !__STDIO_H__ */
