// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	void *tmp = (void *) PFTEMP;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	//cprintf("pgfault: %x env:%x\n",addr,thisenv->env_id);
	if (!(err&2 && uvpt[PGNUM(addr)]&PTE_COW))
		panic("Illegal page access: %x %x %x", addr, uvpt[PGNUM(addr)], err);

	//cprintf("pgfault: %x %x\n",addr,thisenv->env_id);
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	if ((r = sys_page_alloc(0, tmp, PTE_U|PTE_W|PTE_P)) < 0)
		panic("sys_page_alloc err: %e", r);

	addr = ROUNDDOWN(addr, PGSIZE);
	memcpy(tmp, addr, PGSIZE);
	if ((r = sys_page_map(0, tmp, 0, addr, PTE_W|PTE_U|PTE_P)) < 0)
		panic("sys_page_map err: %e", r);
	if ((r = sys_page_unmap(0, tmp)) < 0)
		panic("sys_page_unmap err: %e", r);

	//panic("pgfault not implemented");

}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	void *addr = (void*)(pn * PGSIZE);

	// LAB 4: Your code here.
	//panic("duppage not implemented");
	if (pn > PGNUM(UTOP))
		panic("address is over UTOP.");
	if (!(uvpt[pn] & PTE_U))
		panic("address is not accessable by user.");
	int perm = PTE_U | PTE_P;
	if (uvpt[pn] & (PTE_W|PTE_COW)) {
		perm |= PTE_COW;
		if (!(r = sys_page_map(0, addr, envid, addr, perm)))
			return sys_page_map(0, addr, 0, addr, perm);
		return r;
	}

	return sys_page_map(0, addr, envid, addr, perm);
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	int i, j, r, t;
	set_pgfault_handler(pgfault);

	envid_t child = sys_exofork();

	if (child < 0)
		panic("sys_exofork err: %e", child);

	if (!child) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for	(i = 0; i <= PDX(USTACKTOP); i++) {
		if (!uvpd[i]) continue;
		t = i << 10;
		for (j=0; j<0x400; j++) {
			if (!uvpt[t+j]) continue;
			if (t+j >= PGNUM(USTACKTOP)) break;
			duppage(child, t+j);
		}
	}

	if ((r = sys_page_alloc(child,
		(void*)(UXSTACKTOP - PGSIZE), PTE_U|PTE_W|PTE_P)))
		panic("sys_page_alloc err: %e", r);

	sys_env_set_pgfault_upcall(child, thisenv->env_pgfault_upcall);

	if ((r = sys_env_set_status(child, ENV_RUNNABLE)))
		panic("sys_env_set_status err: %e", r);

	return child;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
