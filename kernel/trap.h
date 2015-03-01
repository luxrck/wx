#ifndef __TRAP_H__
#define __TRAP_H__

/* registers as pushed by pusha */
struct pushregs {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t oesp;		/* Useless */
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
} __attribute__((packed));

struct trapframe {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t oesp;		/* Useless */
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint16_t es;
	uint16_t padding1;
	uint16_t ds;
	uint16_t padding2;
	uint32_t trapno;
	/* below here defined by x86 hardware */
	uint32_t err;
	uintptr_t eip;
	uint16_t cs;
	uint16_t padding3;
	uint32_t eflags;
	/* below here only when crossing rings, such as from user to kernel */
	uintptr_t esp;
	uint16_t ss;
	uint16_t padding4;
} __attribute__((packed));

// http://wiki.osdev.org/TSS
struct tss {
	uint32_t	prev;	// Old ts selector
	uintptr_t	esp0;	// Stack pointers and segment selectors
	uint16_t	ss0;	//   after an increase in privilege level
	uint16_t	padding1;
	uintptr_t	esp1;
	uint16_t	ss1;
	uint16_t	padding2;
	uintptr_t	esp2;
	uint16_t	ss2;
	uint16_t	padding3;
	physaddr_t	cr3;	// Page directory base
	uintptr_t	eip;	// Saved state from last task switch
	uint32_t	eflags;
	uint32_t	eax;	// More saved state (registers)
	uint32_t	ecx;
	uint32_t	edx;
	uint32_t	ebx;
	uintptr_t	esp;
	uintptr_t	ebp;
	uint32_t	esi;
	uint32_t	edi;
	uint16_t	es;		// Even more saved state (segment selectors)
	uint16_t	padding4;
	uint16_t	cs;
	uint16_t	padding5;
	uint16_t	ss;
	uint16_t	padding6;
	uint16_t	ds;
	uint16_t	padding7;
	uint16_t	fs;
	uint16_t	padding8;
	uint16_t	gs;
	uint16_t	padding9;
	uint16_t	ldt;
	uint16_t	padding10;
	uint16_t	t;		// Trap on task switch
	uint16_t	iomb;	// I/O map base address
};

// http://wiki.osdev.org/Interrupt_Descriptor_Table#I386_Interrupt_Gate
/*struct gatedesc {
	unsigned off_low	: 16;	// low 16 bits of offset in segment
	unsigned sel		: 16;	// segment selector
	unsigned rsv		: 8;	// unsed 0..7, set to 0
	unsigned type		: 4;	// type(GATE_{INT32,TRAP32,TASK})
	unsigned s			: 1;	// storage segment, set to 0
	unsigned dpl		: 2;	// descriptor(meaning new) privilege level
	unsigned p			: 1;	// Present
	unsigned off_high	: 16;	// high 16 bits of offset in segment
};*/


/* The kernel's interrupt descriptor table */
extern uint64_t idt[];

void trapinit(void);
void print_trapframe(struct trapframe *tf);
void backtrace(struct trapframe *tf);

// Interrupt Handler
void kbd_intr(void);

#endif /* !__TRAP_H__ */
