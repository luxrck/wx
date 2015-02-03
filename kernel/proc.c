#include <x86.h>
#include <elf.h>
#include <errno.h>
#include <string.h>

#include <kernel/kernel.h>

#include <kernel/mm.h>
#include <kernel/proc.h>
#include <kernel/trap.h>

// ap: 0 system,		1 application
// gr: 0 block = 1B,	1 block = 4K
// sz: 0 16bit segment,	1 32bit segment
#define SEG(type, base, lim, dpl, ap, gr, sz)							\
(																		\
	((gr ? (UL(lim) >> 12) : UL(lim)) & 0xFFFF)						|	\
	((UL(base) & 0xFFFFFF)									<< 16)	|	\
	(((1 << 7) | (UL(dpl) << 5) | (UL(ap) << 4) | UL(type))	<< 40)	|	\
	(((gr ? (UL(lim) >> 28) : (UL(lim) >> 16)) & 0xF)		<< 48)	|	\
	(((UL(gr) << 3) | (UL(sz) << 2))						<< 52)	|	\
	(((UL(base) >> 24) & 0xFF)								<< 56)		\
)

struct tss tss;
uint64_t gdt[NCPU + 5];

#define UINITPATH	"/init"
static struct proc *uinit = NULL;

static void procuserinit(void)
{
	return;
}

void procinit(void)
{
	gdt[0] = 0;						// not used.			[offset|dpl]
	gdt[1] = 0x00CF9A000000FFFF;	// kernel code segment.	[0x01  |00 ]
	gdt[2] = 0x00CF92000000FFFF;	// kernel data segment.	[0x02  |00 ]
	gdt[3] = 0x00CFFA000000FFFF;	// user code segment.	[0x03  |11 ]
	gdt[4] = 0x00CFF2000000FFFF;	// user data segment.	[0x04  |11 ]
	// We don't support SMP, so we just set TSS0.
	// We create TSS for each CPU and uses them for all tasks.
	tss.ss0 = GD_KD;
	tss.esp0 = KSTACKTOP;
	gdt[5] = SEG(STS_T32A, (uint32_t)&tss, sizeof(tss), 0, 0, 0, 1);

	for (int i=1; i < NPROC; i++)
		procs[i-1].next = &procs[i];

	lgdt(gdt, sizeof(gdt));
	asm volatile("movw %%ax,%%gs" :: "a" (GD_UD|3));
	asm volatile("movw %%ax,%%fs" :: "a" (GD_UD|3));
	asm volatile("movw %%ax,%%es" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ds" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ss" :: "a" (GD_KD));
	asm volatile("ljmp %0,$1f\n 1:\n" :: "i" (GD_KT));
	lldt(0);

	ltr(GD_TSS0);

	procuserinit();
}

struct proc* procalloc(void)
{
	static size_t avaliable_pid = 1;
	if (!procs) return NULL;

	struct page *pp = pagealloc(1);
	if (!pp) return NULL;

	struct proc *task = procs;
	procs = procs->next;
	memset(task, 0, sizeof(struct proc));

	task->pgdir = page2kva(pp);
	vamap(kpgdir, pp, task->pgdir, PTE_W | PTE_P);
	for (int i=PDX(UTOP); i<0x400; i++)
		task->pgdir[i] = kpgdir[i] | PTE_U;

	task->tf.ds = GD_UD | 3;
	task->tf.es = GD_UD | 3;
	task->tf.ss = GD_UD | 3;
	task->tf.cs = GD_UT | 3;
	task->tf.esp = USTACKTOP;
	task->tf.eflags |= FL_IF;

	task->pid = avaliable_pid++;
	task->state = PROC_BLOCKED;
	task->count = 0;

	task->next = tasks;
	tasks = task;

	return task;
}

static int unmappage(int index, pte_t pte, void *pgdir)
{
	if (index >= PGNUM(UTOP)) return 0;
	vaunmap(pgdir, (void*)(index * PGSIZE));
	return 1;
}
void procfree(struct proc *tk)
{
	printf("proc free [%d %d %d] ", tk->pid, tk->state, tk->parent ? tk->parent->pid : 0);
	if (tk->parent)
		tk->parent->state = PROC_RUNNABLE;
	if (tk == ctask) {
		lcr3(PADDR(kpgdir));
		ctask = NULL;
	}

	pageforeach(tk->pgdir, unmappage, tk->pgdir);
	for (int i=0; i<PDX(UTOP); i++) {
		if (!tk->pgdir[i]) continue;
		pagedecref(pa2page(PTE_ADDR(tk->pgdir[i])));
	}
	pagedecref(pa2page(PADDR(tk->pgdir)));
	struct proc *it = tasks;
	if (tasks != tk)
		while (it && it->next != tk) it = it->next;
	else 
		tasks = tasks->next;
	if (it) it->next = tk->next;
	
	memset(tk, 0, sizeof(struct trapframe));

	tk->next = procs;
	procs = tk;
}

void procrestore(struct proc* tk)
{
	if (ctask && ctask->state == PROC_RUNNING)
		ctask->state = PROC_RUNNABLE;

	ctask = tk;
	ctask->state = PROC_RUNNING;

	lcr3(PADDR(ctask->pgdir));

	ctask->count++;

	printf("proc run [%d %d] ", tk->pid, tk->parent ? tk->parent->pid : 0);
	asm volatile("movl %0,%%esp\n\t"
		"popal\n\t"
		"popl %%es\n\t"
		"popl %%ds\n\t"
		"addl $0x8,%%esp\n\t" /* skip tf_trapno and tf_errcode */
		"iret"
		: : "g" (&(ctask->tf)) : "memory");
	panic("iret failed");
}

static int duppage(int index, pte_t pte, void *pgdir)
{
	if (index >= PGNUM(UTOP)) return 0;
	index *= PGSIZE;
	struct page *pp = pagealloc(1);
	memcpy(page2kva(pp), (void*)index, PGSIZE);
	vamap(pgdir, pp, (void*)index, PTE_W|PTE_U|PTE_P);
	return 1;
}
pid_t fork(void)
{
	struct proc *child;
	if (!(child = procalloc())) {
		errno = EAGAIN;
		return -1;
	}
	/*for (int i=0; i<=PDX(USTACKTOP); i++) {
		if (!ctask->pgdir[i]) continue;
		pte_t *pte = KADDR(PTE_ADDR(ctask->pgdir[i]));
		for (int j=0; j<0x400; j++) {
			if (!pte[j]) continue;
			struct page *pp = pagealloc(1);
			uintptr_t va = (i << PTSHIFT) + j * PGSIZE;
			memcpy(page2kva(pp), (void*)va, PGSIZE);
			vamap(child->pgdir, pp, (void*)va, PTE_W|PTE_U|PTE_P);
		}
	}*/
	pageforeach(ctask->pgdir, duppage, child->pgdir);
	child->tf = ctask->tf;
	child->tf.eax = 0;
	child->parent = ctask;
	child->state = PROC_RUNNABLE;
	return child->pid;
}

void exit(void)
{
	printf("proc exit [%d]\n", ctask->pid);
	ctask->state = PROC_ZOMBIE;
	if (ctask->parent && ctask->parent->state == PROC_BLOCKED)
		ctask->parent->state = PROC_RUNNABLE;
	scheduler();
}

pid_t waitpid(pid_t pid)
{
	printf("proc wait [%x] ", ctask->pid);
	struct proc *tk = tasks;
	bool haschild = 0;

tloop:
	while (tk && (!tk->parent || tk->parent->pid != ctask->pid))
		tk = tk->next;
	if (pid != -1 && tk && tk->pid != pid) goto tloop;
	
	if (!haschild && tk) {
		haschild = 1;
		ctask->state = PROC_BLOCKED;
	}

	if (!haschild) {
		errno = ECHILD;
		return -1;
	}

	if (tk)
		printf("<%d>", tk->pid);
	if (tk && tk->state == PROC_ZOMBIE) {
		pid_t pid = tk->pid;
		procfree(tk);
		goto ret;
	}
	if (pid == -1 && tk && (tk = tk->next))
		goto tloop;
	scheduler();
ret:
	ctask->state = PROC_RUNNING;
	return pid;
}

static void
region_alloc(struct proc *e, void *va, size_t len)
{
	// LAB 3: Your code here.
	// (But only if you need it for load_icode.)
	//
	// Hint: It is easier to use region_alloc if the caller can pass
	//   'va' and 'len' values that are not page-aligned.
	//   You should round va down, and round (va + len) up.
	//   (Watch out for corner-cases!)
	char *vb = ROUNDDOWN(va, PGSIZE);
	char *vt = ROUNDUP(va + len, PGSIZE);

	for (; vb<vt; vb+=PGSIZE)
		vamap(e->pgdir, pagealloc(1), vb, PTE_U | PTE_W | PTE_P);
}

static void
load_icode(struct proc *e, uint8_t *binary)
{
	lcr3(PADDR(e->pgdir));
	//panic("aaaaaaaaa");
	struct elf *eheader = (struct elf*)binary;
	struct proghdr *ph = (struct proghdr*)(binary + eheader->phoff);
	struct proghdr *eh = ph + eheader->phnum;
	e->tf.eip = eheader->entry;

	if (ph->filesz > ph->memsz)
		panic("ELF binary memory overflow.");

	struct page *pp;
	pte_t *pte;
	for (; ph<eh; ph++) {
		if (ph->type == ELF_PROG_LOAD) {
			region_alloc(e, (char*)ph->va, ph->memsz);
			memcpy((char*)ph->va, binary + ph->offset, ph->filesz);
		}
	}
	
	size_t pm = ph->memsz;
	pm = pm % PGSIZE ? pm / PGSIZE + 1 : pm / PGSIZE;
	memset((char*)(ph->va+ph->filesz), 0, pm - ph->memsz);

	vamap(e->pgdir, pagealloc(1), (void*)(USTACKTOP-PGSIZE),
		PTE_P | PTE_U | PTE_W);
	lcr3(PADDR(kpgdir));
}

//
// Allocates a new env with env_alloc, loads the named elf
// binary into it with load_icode, and sets its env_type.
// This function is ONLY called during kernel initialization,
// before running the first user-mode environment.
// The new env's parent ID is set to 0.
//
void
env_create(uint8_t *binary)
{
	struct proc *e = procalloc();
	vamap(e->pgdir, pagealloc(1), (void*)(USTACKTOP-PGSIZE), PTE_W | PTE_U | PTE_P);
	load_icode(e, binary);
	e->state = PROC_RUNNABLE;
	e->parent = NULL;
	ctask = e;
}

void
env_pop_tf(struct trapframe *tf)
{
	__asm __volatile("movl %0,%%esp\n"
		"\tpopal\n"
		"\tpopl %%es\n"
		"\tpopl %%ds\n"
		"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
		"\tiret"
		: : "g" (tf) : "memory");
	panic("iret failed");  /* mostly to placate the compiler */
}

//
// Context switch from curenv to env e.
// Note: if this is the first call to env_run, curenv is NULL.
//
// This function does not return.
//
void
env_run(struct proc *e)
{
	// Step 1: If this is a context switch (a new environment is running):
	//	   1. Set the current environment (if any) back to
	//	      ENV_RUNNABLE if it is ENV_RUNNING (think about
	//	      what other states it can be in),
	//	   2. Set 'curenv' to the new environment,
	//	   3. Set its status to ENV_RUNNING,
	//	   4. Update its 'env_runs' counter,
	//	   5. Use lcr3() to switch to its address space.
	// Step 2: Use env_pop_tf() to restore the environment's
	//	   registers and drop into user mode in the
	//	   environment.

	// Hint: This function loads the new environment's state from
	//	e->env_tf.  Go back through the code you wrote above
	//	and make sure you have set the relevant parts of
	//	e->env_tf to sensible values.

	// LAB 3: Your code here.
	if (e != ctask && ctask->state == PROC_RUNNING)
		ctask->state = PROC_RUNNABLE;
	ctask = e;
	e->state = PROC_RUNNING;
	e->count++;
	//printf("proc run %.8x\n", e->pid);
	lcr3(PADDR(e->pgdir));
	
	env_pop_tf(&(e->tf));
}

