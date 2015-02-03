#ifndef __MM_H__
#define __MM_H__

#include <assert.h>

#include <kernel/kernel.h>

#define PGNUM(la)	(((uintptr_t) (la)) >> PTXSHIFT)

// page directory index
#define PDX(la)		((((uintptr_t) (la)) >> PDXSHIFT) & 0x3FF)

// page table index
#define PTX(la)		((((uintptr_t) (la)) >> PTXSHIFT) & 0x3FF)

// offset in page
#define PGOFF(la)	(((uintptr_t) (la)) & 0xFFF)

// construct linear address from indexes and offset
#define PGADDR(d, t, o)	((void*) ((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

#define PTE_ADDR(pte)	((physaddr_t) (pte) & ~0xFFF)

struct page {
	uint32_t ref;
	struct page *next;
} *pages, *pagebase;
size_t npages;
pde_t *kpgdir;

#define PADDR(kva) _paddr(__FILE__, __LINE__, kva)
static inline physaddr_t
_paddr(const char *file, int line, void *kva)
{
	if ((uint32_t)kva < KERNBASE)
		_panic(file, line, "PADDR called with invalid kva %.8x", kva);
	return (physaddr_t)kva - KERNBASE;
}

#define KADDR(pa) _kaddr(__FILE__, __LINE__, pa)
static inline void*
_kaddr(const char *file, int line, physaddr_t pa)
{
	if (PGNUM(pa) >= npages)
		_panic(file, line, "KADDR called with invalid pa %.8x", pa);
	return (void *)(pa + KERNBASE);
}

void meminit(void);

void pageinit(void);
pte_t* pteget(pde_t *pgdir, const void *va, int create);
void pageforeach(pde_t *pgdir, int(*cb)(int index, pte_t pte, void *data), void *data);
int vamap(pde_t *pgdir, struct page *pp, void *va, int perm);
void vaunmap(pde_t *pgdir, void *va);
void varemap(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm);
void pagedecref(struct page *pp);
struct page* pagealloc(bool fill_zero);
struct page* pageget(pde_t *pgdir, void *va, pte_t **pte_store);

void tlb_invalidate(pde_t *pgdir, void *va);

#define page2pa(pp) ((physaddr_t)((struct page*)pp - pagebase) << PGSHIFT)
#define pa2page(pa) (&pages[PGNUM(pa)])
#define page2kva(pp) (KADDR(page2pa(pp)))

#endif /* !__MM_H__ */
