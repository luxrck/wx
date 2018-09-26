#include <x86.h>
#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <kernel/kernel.h>

#include <kernel/mm.h>
#include <kernel/proc.h>
#include <kernel/trap.h>
#include <kernel/device.h>

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

static void __ldelf(struct proc *e, int fd);

static void procuserinit(void)
{
	struct proc *e = procalloc();

	vamap(e->pgdir, pagealloc(1), (void*)(UXSTACKTOP-PGSIZE), PTE_W|PTE_U|PTE_P);

	vamap(e->pgdir, pagealloc(1), (void*)(USTACKTOP-PGSIZE), PTE_W|PTE_U|PTE_P);
	vamap(e->pgdir, pagealloc(1), (void*)(USTACKTOP-PGSIZE*2), PTE_W|PTE_U|PTE_P);

	ctask = e;

	e->state = PROC_RUNNABLE;
	e->parent = NULL;
	e->sb = sbread(rootdev);
	e->root = FS_ROOTI;
	e->cwd = e->root;

	int fd = open(UINITPATH, O_RDONLY);
	if (fd < 0)
		panic("can not open '/init'");
	printf("uinit: %d\n", fd);

	__ldelf(e, fd);

	close(fd);
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
	tss.esp0 = UXSTACKTOP;
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
	task->state = PROC_SLEEPING;
	task->count = 0;

	task->next = tasks;
	tasks = task;

	return task;
}

static int __unmappage(pde_t *pgdir, int index, pte_t pte, void *data)
{
	if (index >= PGNUM(UTOP)) return 0;
	vaunmap(pgdir, (void*)(index * PGSIZE));
	return 1;
}
void procfree(struct proc *tk)
{
	if (tk->parent)
		tk->parent->state = PROC_RUNNABLE;
	if (tk == ctask) {
		//lcr3(PADDR(kpgdir));
		ctask = NULL;
		return;
	}

	pageforeach(tk->pgdir, __unmappage, NULL);
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

	tk->root = tk->cwd = 0;
	tk->sb = 0;

	tk->next = procs;
	procs = tk;
}

// MUST NOT BE INLINE!
// THIS IMPLEMENTATION RELAY ON C'S CALL STACK.
void procsavecontext(struct procexec *ec)
{
	asm volatile(
		// In this function, C didn't help us to push & modify %ebp, %esp automatically.
		// So I did this manually.
		"push	%%ebp\n\t"
		"movl	%%esp,      %%ebp\n\t"

        "movl	8(%%ebp),	%%esp\n\t"
		"addl	$36,		%%esp\n\t"
		"pusha\n\t"
		"movl	(%%ebp),	%%eax\n\t"
		"movl	%%eax,		8(%%esp)\n\t"	// save oebp
		"movl	%%ebp,		12(%%esp)\n\t"	// save oesp
		"addl	$8,			12(%%esp)\n\t"
		"movl	4(%%ebp),	%%eax\n\t"
		"pushl	%%eax\n\t"
		//"cmpl	$0,			0xc(%%ebp)\n\t"
		//"je		1f\n\t"
		//"movl	0xc(%%ebp),	%%eax\n\t"
		//"movl	%%eax,		(%%esp)\n"		// recall parent function
		//"movl (%%ebp),	16(%%esp)"
		//"1:\n\t"
		"movl	%%ebp,		%%esp\n\t"
		"pop    %%ebp\n\t"
		"ret\n\t"
		::: "memory" );
}

void procrestorecontext(struct procexec *ec)
{
	asm volatile(
		"push	%%ebp\n\t"
		"movl	%%esp,		%%ebp\n\t"
		"movl	8(%%ebp),	%%esp\n\t"
		"addl	$4,			%%esp\n\t"
		"popal\n\t"
		"movl	%%esp,		%%eax\n\t"
		"movl	-20(%%eax),	%%esp\n\t"
		"jmp	*-36(%%eax)\n\t"

		// Never reach here.
		"ret\n\t"
		::: "memory" );
}

void procrestore(struct proc* tk)
{
	if (ctask && ctask->state == PROC_RUNNING)
		ctask->state = PROC_RUNNABLE;

	ctask = tk;
	ctask->state = PROC_RUNNING;

	lcr3(PADDR(ctask->pgdir));

	ctask->count++;

	if (ctask->ec.eip)
		procrestorecontext(&ctask->ec);

	asm volatile("movl %0, %%esp\n\t"
		"popal\n\t"
		"popl %%es\n\t"
		"popl %%ds\n\t"
		"addl $0x8,%%esp\n\t" /* skip tf_trapno and tf_errcode */
		"iret"
		: : "g" (&(ctask->tf)) : "memory");
	panic("iret failed");
}

static int __duppage(pde_t *pgdir, int index, pte_t pte, void *data)
{
	if (index >= PGNUM(USTACKTOP)) return 0;
	index *= PGSIZE;
	struct page *pp = pagealloc(1);
	memcpy(page2kva(pp), (void*)index, PGSIZE);
	vamap(data, pp, (void*)index, PTE_W|PTE_U|PTE_P);
	return 1;
}
pid_t fork(void)
{
	struct proc *child;
	if (!(child = procalloc()))
		return -EAGAIN;

	pageforeach(ctask->pgdir, __duppage, child->pgdir);

	vamap(child->pgdir, pagealloc(1), (void*)(UXSTACKTOP-PGSIZE), PTE_W|PTE_U|PTE_P);
	vamap(child->pgdir, pagealloc(1), (void*)(UXSTACKTOP-PGSIZE*2), PTE_W|PTE_U|PTE_P);

	child->tf = ctask->tf;
	child->tf.eax = 0;
	child->parent = ctask;
	child->state = PROC_RUNNABLE;

	child->etext = ctask->etext;
	child->edata = ctask->edata;
	child->end = ctask->end;
	child->brk = ctask->brk;

	child->sb = ctask->sb;
	child->root = ctask->root;
	child->cwd = ctask->cwd;

	for (int i=0; i<NPOF; i++) {
		if (ctask->filp[i]) {
			child->filp[i] = ctask->filp[i];
			ctask->filp[i]->ref++;
		}
	}

	return child->pid;
}

static void __elfsegmap(struct proc *e, void *va, size_t len)
{
	char *vb = ROUNDDOWN(va, PGSIZE);
	char *vt = ROUNDUP(va + len, PGSIZE);

	for (; vb<vt; vb+=PGSIZE) {
		if (!pteget(e->pgdir, vb, 0))
			vamap(e->pgdir, pagealloc(1), vb, PTE_U | PTE_W | PTE_P);
		else
			memset(vb, 0, PGSIZE);
	}
}
static void __elfsegldd(int fd, void *va, off_t offset, size_t count)
{
	struct page *pp = pagealloc(1);
	char *binary = (char*)page2kva(pp);

	lseek(fd, offset, SEEK_SET);

	int r = 0;
	for (int i=0; i<count/PGSIZE; i++, va+=PGSIZE) {
		r = read(fd, binary, PGSIZE);
		if (r < 0)
			panic("__elfsegldd: %d", r);
		memcpy(va, binary, PGSIZE);
	}

	read(fd, binary, count % PGSIZE);
	//printf("__elfsegldd.3: %x %x\n", va, count % PGSIZE);
	memcpy(va, binary, count % PGSIZE);

	pagedecref(pp);
}
static void __ldelf(struct proc *e, int fd)
{
	lcr3(PADDR(e->pgdir));

	struct page *pp = pagealloc(1);
	char *binary = (char*)page2kva(pp);

	int r = read(fd, binary, PGSIZE);
	if (r < 0)
		panic("__ldelf: %d", r);

	struct elf *eheader = (struct elf*)binary;
	struct proghdr *ph = (struct proghdr*)(binary + eheader->phoff);
	struct proghdr *eph = ph + eheader->phnum;

	if (eheader->magic != ELF_MAGIC)
		panic("Bad ELF file: %x", eheader->magic);

	e->tf.eip = eheader->entry;

	if (ph->filesz > ph->memsz)
		panic("ELF binary memory overflow.");

	for (; ph<eph; ph++) {
		if (ph->type == ELF_PROG_LOAD) {
			__elfsegmap(e, (char*)ph->va, ph->memsz);
			__elfsegldd(fd, (void*)ph->va, ph->offset, ph->filesz);
		}
	}

	size_t pm = ROUNDUP(ph->memsz, PGSIZE);
	memset((char*)(ph->va+ph->filesz), 0, pm - ph->memsz);

	lseek(fd, eheader->shoff, SEEK_SET);
	uint16_t ssndx = eheader->shstrndx;
	uint16_t shnum = eheader->shnum;

	read(fd, binary, 2048);
	char *binary1 = binary + 2048;

	struct secthdr *sh = (struct secthdr*)binary;
	struct secthdr *ss = sh + ssndx;

	lseek(fd, ss->offset, SEEK_SET);
	read(fd, binary1, 2048);

	// .text .rodata .data .bss
	for (int i=1; i<shnum; i++) {
		if (i == ssndx)
			continue;
		char *name = binary1 + sh[i].name;
		uintptr_t eaddr = sh[i].addr + sh[i].size;
		if (!strcmp(name, ".text"))
			e->etext = eaddr;
		if (!strcmp(name, ".data"))
			e->edata = eaddr;
		if (!strcmp(name, ".bss")) {
			e->end = eaddr;
			if (!e->edata)
				e->edata = sh[i].addr;
		}
	}

	if (e->end) {
		e->brk = ROUNDUP(e->end, PGSIZE);
		vamap(e->pgdir, pagealloc(1), (void*)e->brk, PTE_P | PTE_U | PTE_W);
	}

	pagedecref(pp);
}
int execv(const char *path, char *argv[])
{
	int fd = open(path, O_RDONLY);
	if (fd < 0) return fd;

	ctask->tf.esp = USTACKTOP;
	char **cargv = NULL;

	if (!argv)
		goto noargs;

	int i = 0, l = 0;
	while (argv[i]) {
		l += strlen(argv[i]) + 1;
		i++;
	}

	// GCC'S STACK ALIGNMENT
	// -mpreferred-stack-boundary=num
	// Attempt to keep the stack boundary aligned to a 2 raised to num byte
	// boundary. If -mpreferred-stack-boundary is not specified, the default
	// is 4 (16 bytes or 128 bits).
	//
	// We use -mpreferred-stack-boundary=2 for user apps.
	//
	// When we enter in user:main, gcc will align the stack. So if we don't
	// set this, we probably will get wrong args for user main function.
	//
	// Remember to modify user/Makefile if you modify this.
	l = ROUNDUP(l, 4);

	char *cargs = (char*)USTACKTOP - l;
	*(int*)(cargs - 4) = 0;
	cargv = (char**)(cargs - 4 - i*4);
	*(char***)(cargv - 1) = cargv;
	*(int*)(cargv - 2) = i;

	for (int j=0; j<i; j++) {
		int k = strlen(argv[j]) + 1;
		memcpy(cargs, argv[j], k);
		cargv[j] = cargs;
		cargs += k;
	}

	ctask->tf.esp = (uintptr_t)cargv - 8;

noargs:
	ctask->tf.ebp = 0;
	ctask->tf.eax = 0;

	__ldelf(ctask, fd);

	close(fd);

	procrestore(ctask);

	return 0;
}

void exit(void)
{
	ctask->state = PROC_ZOMBIE;

	for (int i=0; i<NPOF; i++)
		if (ctask->filp[i] && close(i)){}

	struct proc *it = tasks, *ii = procget(PID_INIT);
	while (it) {
		if (it->parent == ctask)
			it->parent = ii;
		it = it->next;
	}

	if (ctask->parent)
		wakeup(ctask->parent);

	scheduler();
}

void sleep(void *chan)
{
	ctask->chan = chan;
	ctask->state = PROC_SLEEPING;

	procsavecontext(&ctask->ec);

	if (ctask->state == PROC_SLEEPING)
		scheduler();

	ctask->chan = 0;
	ctask->ec.eip = 0;
}

void wakeup(void *chan)
{
	struct proc *it = tasks;
	while (it) {
		if (it->state == PROC_SLEEPING && it->chan == chan) {
			it->state = PROC_RUNNABLE;
		}
		it = it->next;
	}
	scheduler();
}

pid_t waitpid(pid_t pid, int options)
{
	struct proc *tk = tasks;
	bool haschild = 0;

tloop:
	while (tk && (!tk->parent || tk->parent->pid != ctask->pid))
		tk = tk->next;
	if (pid != -1 && tk && tk->pid != pid) goto tloop;

	if (!haschild && tk) {
		haschild = 1;
		ctask->state = PROC_SLEEPING;
	}

	if (!haschild) {
		pid = -ECHILD;
		goto err;
	}

	if (tk && tk->state == PROC_ZOMBIE) {
		pid_t pid = tk->pid;
		procfree(tk);
		goto ret;
	}

	if (pid == -1 && tk && (tk = tk->next))
		goto tloop;

	if (options & WNOHANG) {
		pid = 0;
		goto ret;
	}

	sleep(ctask);

	tk = tasks;
	goto tloop;

ret:
	ctask->state = PROC_RUNNING;
err:
	return pid;
}

int brk(void *addr)
{
	if ((uintptr_t)addr < ctask->end || (uintptr_t)addr >= USTACKTOP)
		return -ENOMEM;
	void *vb = ROUNDDOWN((void*)ctask->brk, PGSIZE) + PGSIZE;
	void *vt = ROUNDDOWN(addr, PGSIZE);
	for (; vb<=vt; vb+=PGSIZE)
		vamap(ctask->pgdir, pagealloc(1), vb, PTE_P | PTE_U | PTE_W);
	ctask->brk = (uintptr_t)addr;
	return 0;
}

void* sbrk(int inc)
{
	uintptr_t ob = ROUNDDOWN(ctask->brk, PGSIZE);
	uintptr_t nb = ROUNDDOWN(ctask->brk + inc, PGSIZE);
	uintptr_t brk = ctask->brk;
	ctask->brk += inc;

	if (nb == ob)
		goto ret;
	else if (nb > ob) {
		for (int i=ob+1; i<=nb; i+=PGSIZE)
			vamap(ctask->pgdir, pagealloc(1), (void*)i, PTE_P | PTE_U | PTE_W);
	} else {
		for (int i=ob; i>nb; i-=PGSIZE)
			vaunmap(ctask->pgdir, (void*)i);
	}

ret:
	return (void*)brk;
}
