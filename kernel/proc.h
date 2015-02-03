#ifndef __PROC_H__
#define __PROC_H__

#include <stdio.h>
#include <kernel/trap.h>

extern uint64_t gdt[NCPU + 5];

enum proc_state { PROC_UNUSED = 0, PROC_RUNNABLE, PROC_RUNNING, PROC_BLOCKED, PROC_SLEEPING, PROC_ZOMBIE };

struct proc {
	struct trapframe tf;	// Saved registers
	struct proc *next;		// Next free proc
	struct proc *parent;	// Pid of this proc's parent
	pid_t pid;				// Unique environment identifier
	enum proc_state state;	// State of the environment
	uint32_t count;			// Number of times environment has run
	pde_t *pgdir;			// Kernel virtual address of page dir
} *procs, *tasks, *ctask;
// procs: avaliable procs; tasks: alloced procs; ctask: current running proc

void procinit(void);
struct proc* procalloc(void);
void procfree(struct proc* tk);
void procrestore(struct proc* tk) __attribute__((noreturn));

pid_t fork(void);
pid_t waitpid(pid_t pid);
void exit(void);

void	env_create(uint8_t *binary);
void	env_run(struct proc *e) __attribute__((noreturn));
void	env_pop_tf(struct trapframe *tf) __attribute__((noreturn));


// Without this extra macro, we couldn't pass macros like TEST to
// ENV_CREATE because of the C pre-processor's argument prescan rule.
#define ENV_PASTE3(x, y, z) x ## y ## z

#define ENV_CREATE(x)						\
	do {								\
		extern uint8_t ENV_PASTE3(_binary____, x, _start)[];	\
		env_create(ENV_PASTE3(_binary____, x, _start));					\
	} while (0)

#endif /* !__PROC_H__ */
