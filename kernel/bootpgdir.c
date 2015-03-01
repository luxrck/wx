#include <stdio.h>
#include <kernel/mm.h>
#include <kernel/kernel.h>

// We should set this explicitly otherwise bootpgtable will be
// puted in the .bss section and of course we don't want it because
// we set them zero in init.c:i386_init
__attribute__((__aligned__(PGSIZE))) pte_t bootpgtable[NPTE] = {
	[0] = 0x000000 | PTE_P | PTE_W
};

// The entry.S page directory maps the first 4MB of physical memory
// starting at virtual address KERNBASE (that is, it maps virtual
// addresses [KERNBASE, KERNBASE+4MB) to physical addresses [0, 4MB)).
// We choose 4MB because that's how much we can map with one page
// table and it's enough to get us through early boot.  We also map
// virtual addresses [0, 4MB) to physical addresses [0, 4MB); this
// region is critical for a few instructions in entry.S and then we
// never use it again.
//
// Page directories (and page tables), must start on a page boundary,
// hence the "__aligned__" attribute.  Also, because of restrictions
// related to linking and static initializers, we use "x + PTE_P"
// here, rather than the more standard "x | PTE_P".  Everywhere else
// you should use "|" to combine flags.
__attribute__((__aligned__(PGSIZE))) pde_t bootpgdir[NPDE] = {
	// Map VA's [0, 4MB) to PA's [0, 4MB)
	[0]
		= ((uintptr_t)bootpgtable - KERNBASE) + PTE_P,
	// Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
	[KERNBASE>>PDXSHIFT]
		= ((uintptr_t)bootpgtable - KERNBASE) + PTE_P + PTE_W
};