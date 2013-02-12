#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <vmstats.h>
#include <array.h>
#include <coremap.h>
#include "opt-A3.h"

#if OPT_A3

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	as->as_vbase1 = 0;
	as->as_pbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = 0;
	as->as_npages2 = 0;
	as->as_stackpbase = 0;

	as->pt = array_create();

	return as;
}

void
as_destroy(struct addrspace *as)
{
	array_destroy(as->pt);
	kfree(as);
}

void
as_activate(struct addrspace *as)
{
	int i, spl;

	(void)as;

	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	_vmstats_inc(VMSTAT_TLB_INVALIDATE);

	splx(spl);
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	unsigned int i;
	struct pte * e;

	for (i = 0; i < npages; i++)
	{
		e = kmalloc(sizeof(struct pte));
		e->vaddr = vaddr + i * PAGE_SIZE;
		e->flags = readable | writeable | executable;
		e->valid = 0;
		array_add(as->pt, e);
	}

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;
}

int
as_prepare_load(struct addrspace *as)
{
	assert(as->as_pbase1 == 0);
	assert(as->as_pbase2 == 0);
	assert(as->as_stackpbase == 0);

/* DONT ALLOCATE PHYSICAL MEMORY RIGHT AWAY

	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0) {
		return ENOMEM;
	}

	as->as_pbase2 = getppages(as->as_npages2);
	if (as->as_pbase2 == 0) {
		return ENOMEM;
	}

	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}

*/

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	//assert(as->as_stackpbase != 0);

	struct pte * e;
	int i;

	for (i = 0; i < DUMBVM_STACKPAGES; i++)
	{
		e = kmalloc(sizeof(struct pte));
		e->vaddr = USERSTACK - i * PAGE_SIZE;
		e->flags = 0x7;
		e->valid = 0;
		array_add(as->pt, e);
	}

	*stackptr = USERSTACK;
	return 0;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;

	unsigned int i;
	struct pte * e;

	new->as_pbase1 = getppages(new->as_npages1);
	if (new->as_pbase1 == 0) {
		return ENOMEM;
	}

	assert(new->as_pbase1 != 0);

	for (i = 0; i < new->as_npages1; i++)
	{
		e = kmalloc(sizeof(struct pte));
		e->vaddr = new->as_vbase1 + i * PAGE_SIZE;
		e->paddr = new->as_pbase1 + i * PAGE_SIZE;
		e->valid = 1;
		e->flags = 0x5;
		array_add(new->pt, e);
	}

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
		old->as_npages1*PAGE_SIZE);

	new->as_pbase2 = getppages(new->as_npages2);
	if (new->as_pbase2 == 0) {
		return ENOMEM;
	}

	assert(new->as_pbase2 != 0);

	for (i = 0; i < new->as_npages2; i++)
	{
		e = kmalloc(sizeof(struct pte));
		e->vaddr = new->as_vbase2 + i * PAGE_SIZE;
		e->paddr = new->as_pbase2 + i * PAGE_SIZE;
		e->valid = 1;
		e->flags = 0x6;
		array_add(new->pt, e);
	}

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
		old->as_npages2*PAGE_SIZE);

	new->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	if (new->as_stackpbase == 0) {
		return ENOMEM;
	}

	assert(new->as_stackpbase != 0);

	for (i = 0; i < DUMBVM_STACKPAGES; i++)
	{
		e = kmalloc(sizeof(struct pte));
		e->vaddr = USERSTACK - i * PAGE_SIZE;
		e->paddr = new->as_stackpbase + i * PAGE_SIZE;
		e->valid = 1;
		e->flags = 0x7;
		array_add(new->pt, e);
	}

	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		DUMBVM_STACKPAGES*PAGE_SIZE);
	
	/*
	for (i = 0; i < array_getnum(new->pt); i++)
	{
		e = (struct pte *) array_getguy(new->pt, i);
		DEBUG(DB_VM, "PT(%d) V 0x%x P 0x%x\n", i+1, e->vaddr, e->paddr);
	}
	*/

	*ret = new;
	return 0;
}

#else

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	/*
	 * Initialize as needed.
	 */

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */

	(void)old;
	
	*ret = newas;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */
	
	kfree(as);
}

void
as_activate(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;  // suppress warning until code gets written
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	/*
	 * Write this.
	 */

	(void)as;
	(void)vaddr;
	(void)sz;
	(void)readable;
	(void)writeable;
	(void)executable;
	return EUNIMP;
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	(void)as;

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	return 0;
}

#endif
