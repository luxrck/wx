#ifndef __LIBC__
#define __LIBC__

void sys_hello(void);

void sys_a(void);
void sys_b(void);
pid_t sys_fork(void);
pid_t sys_waitpid(pid_t pid);
void sys_wait(void);
void sys_exit(void);
pid_t sys_getpid(void);
pid_t sys_getppid(void);
time_t sys_time(time_t *t);

#endif /* !__LIBC__ */