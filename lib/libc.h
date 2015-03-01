#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include <kernel/kernel.h>

#define syscall0(rtype, name)	\
rtype name(void)				\
{								\
	int ret;					\
	asm volatile("int %1\n"		\
		: "=a"(ret)				\
		: "i"(T_SYSCALL),		\
		  "a"(SYS_##name)		\
		: "cc", "memory");		\
	if (ret >= 0)				\
		return (rtype) ret;		\
	errno = -ret;				\
	return -1;					\
}

#define syscall1(rtype, name, t1, a1)	\
rtype name(t1 a1)				\
{								\
	int ret;					\
	asm volatile("int %1\n"		\
		: "=a"(ret)				\
		: "i"(T_SYSCALL),		\
		  "a"(SYS_##name),		\
		  "b"(a1)				\
		: "cc", "memory");		\
	if (ret >= 0)				\
		return (rtype) ret;		\
	errno = -ret;				\
	return (rtype) -1;			\
}

#define syscall2(rtype, name, t1, a1, t2, a2)	\
rtype name(t1 a1, t2 a2)		\
{								\
	int ret;					\
	asm volatile("int %1\n"		\
		: "=a"(ret)				\
		: "i"(T_SYSCALL),		\
		  "a"(SYS_##name),		\
		  "b"(a1),				\
		  "c"(a2)				\
		: "cc", "memory");		\
	if (ret >= 0)				\
		return (rtype) ret;		\
	errno = -ret;				\
	return (rtype) -1;			\
}

#define syscall3(rtype, name, t1, a1, t2, a2, t3, a3)	\
rtype name(t1 a1, t2 a2, t3 a3)	\
{								\
	int ret;					\
	asm volatile("int %1\n"		\
		: "=a"(ret)				\
		: "i"(T_SYSCALL),		\
		  "a"(SYS_##name),		\
		  "b"(a1),				\
		  "c"(a2),				\
		  "d"(a3)				\
		: "cc", "memory");		\
	if (ret >= 0)				\
		return (rtype) ret;		\
	errno = -ret;				\
	return (rtype) -1;			\
}
