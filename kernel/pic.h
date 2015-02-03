#ifndef __PICIRQ_H__
#define __PICIRQ_H__

#define MAX_IRQS	16	// Number of IRQs
#define IRQ_SLAVE	2	// IRQ at which slave connects to master

#ifndef __ASSEMBLER__

#include <x86.h>

#include <kernel/kernel.h>

void picinit(void);
void irqset(uint16_t mask);

#endif /* !__ASSEMBLER__ */

#endif /* !__PICIRQ_H__ */
