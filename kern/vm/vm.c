#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include "opt-A3.h"
#include <vmstats.h>
#include <syscall.h>
#include <array.h>
#include <coremap.h>

/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 */

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12

#if OPT_A3
static unsigned int next_victim = 0;
#endif

void
vm_bootstrap(void)
{
	vmstats_init();
}

void
vm_shutdown(void)
{
	_vmstats_print();	
}

#if OPT_A3
int tlb_get_rr_victim()
{
	int victim;

	victim = next_victim % NUM_TLB;
	if (next_victim < NUM_TLB)
	{
		_vmstats_inc(VMSTAT_TLB_FAULT_FREE);
	}
	else
	{
		_vmstats_inc(VMSTAT_TLB_FAULT_REPLACE);
	}
	next_victim++;
	return victim;
}
#endif

/*static*/
/*paddr_t*/
/*getppages(unsigned long npages)*/
/*{*/
/*	int spl;*/
/*	paddr_t addr;*/

/*	spl = splhigh();*/

/*	addr = ram_stealmem(npages);*/
/*	*/
/*	splx(spl);*/
/*	return addr;*/
/*}*/

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(int npages)
{
	paddr_t paddr = getppages(npages);
	if (paddr==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(paddr);
}

void 
free_kpages(vaddr_t vaddr)
{
	releasepages(KVADDR_TO_PADDR(vaddr));
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	u_int32_t ehi, elo;
	struct addrspace *as;
	int spl;

	spl = splhigh();

	faultaddress &= PAGE_FRAME;

	//DEBUG(DB_VM, "dumbvm: type: %d @ 0x%x\n", faulttype, faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		#if OPT_A3
		sys__exit(-1);
		#else
		/* We always create pages read-write, so we can't get this */
		panic("dumbvm: got VM_FAULT_READONLY\n");
		#endif
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		splx(spl);
		return EINVAL;
	}

	as = curthread->t_vmspace;
	if (as == NULL) {
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	#if OPT_A3
	struct pte * e;
	for (i = 0; i < array_getnum(as->pt); i++)
	{
		e = (struct pte *) array_getguy(as->pt, i);

		if (e->vaddr == faultaddress)
		{
			if(e->valid == 0)
			{
				_vmstats_inc(VMSTAT_PAGE_FAULT_DISK);
				paddr = getppages(1);
				if (paddr == NULL)
				{
					return ENOMEM;
				}
				e->paddr = paddr;
				e->valid = 1;

				if (as->as_pbase1 == 0 && faultaddress >= as->as_vbase1 && faultaddress < as->as_vbase1 + as->as_npages1 * PAGE_SIZE)
				{
					as->as_pbase1 = paddr;
				}
				if (as->as_pbase2 == 0 && faultaddress >= as->as_vbase2 && faultaddress < as->as_vbase2 + as->as_npages2 * PAGE_SIZE)
				{
					as->as_pbase2 = paddr;
				}
				if (as->as_stackpbase == 0 && faultaddress >= USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE && faultaddress < USERSTACK)
				{
					as->as_stackpbase = paddr;
				}
				//DEBUG(DB_VM, "VM: Allocated 0x%x at physical address 0x%x\n", faultaddress, paddr);
			}
			else
			{
				_vmstats_inc(VMSTAT_TLB_RELOAD);
				paddr = e->paddr;
			}
			break;
		}
	}
	#endif

	#if OPT_A3
	_vmstats_inc(VMSTAT_TLB_FAULT);

	int dirty = ((e->flags & 0x2)? TLBLO_DIRTY : 0);

	ehi = faultaddress;
	elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	if (dirty & TLB_Probe(ehi, elo) >= 0)
	{
		i = TLB_Probe(ehi, elo);
	}
	else
	{
		i = tlb_get_rr_victim();
	}
	//DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
	TLB_Write(ehi, elo, i);
	splx(spl);
	return 0;
	#else
	for (i=0; i<NUM_TLB; i++) {
		TLB_Read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		//DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		TLB_Write(ehi, elo, i);
		splx(spl);
		return 0;
	}
	#endif

	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
	return EFAULT;
}
