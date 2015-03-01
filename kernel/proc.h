#ifndef __PROC_H__
#define __PROC_H__

#include <stdio.h>

#include <kernel/fs.h>
#include <kernel/trap.h>

#define PID_INIT	1

extern uint64_t gdt[NCPU + 5];

enum proc_state { PROC_UNUSED = 0, PROC_RUNNABLE, PROC_RUNNING, PROC_BLOCKED, PROC_SLEEPING, PROC_ZOMBIE };

struct procexec {
	uint32_t eip;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;	// Actually we didn't save this
};

struct proc {
	struct procexec ec;
	struct trapframe tf;		// Saved registers
	struct proc *next;			// Next free proc
	struct proc *parent;		// Pid of this proc's parent
	pid_t pid;					// Unique environment identifier
	enum proc_state state;		// State of the environment
	void *chan;
	uint32_t count;				// Number of times environment has run
	uint32_t etext,edata,end,brk;
	struct superblock *sb;
	uint16_t cwd, root;
	struct file *filp[NPOF];	// Files opened by this proccess
	pde_t *pgdir;				// Kernel virtual address of page dir
} *procs, *tasks, *ctask;
// procs: avaliable procs; tasks: alloced procs; ctask: current running proc

void procinit(void);
struct proc* procalloc(void);
void procfree(struct proc* tk);
void procrestore(struct proc* tk) __attribute__((noreturn));
void procsavecontext(struct procexec *ec);
void procrestorecontext(struct procexec *ec);

void sleep(void *chan);
void wakeup(void *chan);

static inline struct proc* procget(pid_t pid)
{
	struct proc *it = tasks;
	if (!it) return NULL;
	while (it && it->pid != pid) it = it->next;
	return it;
}

#endif /* !__PROC_H__ */
