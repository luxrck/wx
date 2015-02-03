#include <x86.h>
#include <assert.h>

#include <kernel/kernel.h>

#include <kernel/mm.h>
#include <kernel/proc.h>
#include <kernel/trap.h>
#include <kernel/kshell.h>

uint64_t idt[256];

#define GATE(type, sel, off, dpl)		\
(										\
	(UL(off) & 0xFFFF)				|	\
	(UL(sel) 				<< 16)	|	\
	((UL(type) & 0xF)		<< 40)	|	\
	(((UL(dpl) << 1) | 0x08)<< 44)	|	\
	((UL(off) >> 16)		<< 48)		\
)

void trapinit(void)
{
	extern uint32_t tcb[];

	for (int i=0; i<=0xFF; i++)
		idt[i] = GATE(STS_IG32, GD_KT, tcb[i], 0);
	idt[T_BRKPT] = GATE(STS_IG32, GD_KT, tcb[T_BRKPT], 3);
	idt[T_SYSCALL] = GATE(STS_IG32, GD_KT, tcb[T_SYSCALL], 3);

	lidt(idt, sizeof(idt));
}

void print_trapframe(struct trapframe *tf)
{
	printf("---- trapframe ----\n");
	printf("edi    %.8x\n", tf->edi);
	printf("esi    %.8x\n", tf->esi);
	printf("ebp    %.8x\n", tf->ebp);
	printf("oesp   %.8x\n", tf->oesp);
	printf("ebx    %.8x\n", tf->ebx);
	printf("edx    %.8x\n", tf->edx);
	printf("ecx    %.8x\n", tf->ecx);
	printf("eax    %.8x\n", tf->eax);
	printf("eax    %.8x\n", tf->eax);
	printf("es     %.8x\n", tf->es);
	printf("ds     %.8x\n", tf->ds);
	printf("trapno %.8x\n", tf->trapno);
	printf("err    %.8x\n", tf->err);
	printf("eip    %.8x\n", tf->eip);
	printf("cs     %.8x\n", tf->cs);
	printf("eflags %.8x\n", tf->eflags);
	printf("esp    %.8x\n", tf->esp);
	printf("ss     %.8x\n", tf->ss);
}

void trap(struct trapframe *tf)
{
	if ((tf->cs & 3) == 3) {
		ctask->tf = *tf;
		tf = &ctask->tf;
	}

	switch (tf->trapno) {
	case T_BRKPT:
		kshell();
		break;
	case T_PGFLT:
		print_trapframe(tf);
		//printf("<PGFLT>");
		kshell();
		break;
	case T_SYSCALL:
		tf->eax = syscall(tf->eax, tf->ebx, tf->ecx, tf->edx,
			tf->esi, tf->edi);
		break;

	// Handle spurious interrupts
	// The hardware sometimes raises these because of noise on the
	// IRQ line or other reasons. We don't care.
	case IRQ_OFFSET + IRQ_SPURIOUS:
		if (tf->trapno == IRQ_OFFSET + IRQ_SPURIOUS) {
			printf("Spurious interrupt on irq 7\n");
			//print_trapframe(tf);
		}
		break;

	case IRQ_OFFSET + IRQ_TIMER:
		break;
	
	case IRQ_OFFSET + IRQ_KBD:
		printf("irq_kbd\n");
		break;
	
	default:
		// Unexpected trap: The user process or the kernel has a bug.
		//print_trapframe(tf);
		if (tf->cs == GD_KT) {
			print_trapframe(tf);
			panic("unhandled trap %x in kernel", tf->trapno);
		} else {
			ctask->state = PROC_ZOMBIE;
		}
	}
	scheduler();
}
