#include <coremap.h>
#include <curthread.h>
#include <thread.h>
#include <lib.h>
#if OPT_A3

void initialize_coremap() 
{
	u_int32_t firstpaddr = 0; // address of first free physical page 
	u_int32_t lastpaddr = 0; // one past end of last free physical page
	ram_getsize(&firstpaddr, &lastpaddr);

	coremap_size = (lastpaddr-firstpaddr)/PAGE_SIZE;
	coremap = kmalloc(sizeof(struct coremap_entry*) * coremap_size);
	if (coremap == NULL) {
		panic("\ncoremap: Unable to create\n");
	}
	
	int i;
	for (i = 0; i < coremap_size; i++) {
		struct coremap_entry *entry = kmalloc(sizeof(struct coremap_entry));
		entry->paddr = firstpaddr + (i * PAGE_SIZE);
		entry->used = 0;
		entry->block_len = -1;
		coremap[i] = entry;
	}
	
	coremap_ready = 1;
	
	// Get the latest used ram
	ram_getsize(&firstpaddr, &lastpaddr);

	// "Fill up" the core map with the ram already used
	for(i = 0; coremap[i]->paddr < firstpaddr; i++) {
		coremap[i]->used = 1;
	}

	kprintf("INITIALIZE COREMAP: %d %d\n", firstpaddr, coremap_size);
}

paddr_t getppages(unsigned long npages)
{
	if (coremap_ready == 0)
		return ram_stealmem(npages);

	int i, j;
	unsigned int count = 0;
	for (i = 0; i < coremap_size; i++) {
		if (coremap[i]->used) {
			count = 0;
		} else {
			count++;
		}
		if (count == npages) {
			coremap[i - npages + 1]->block_len = npages;
			for (j = i - npages + 1; j <= i; j++) {
				coremap[j]->used = 1;
			}

			return coremap[i - npages + 1]->paddr;
		}
	}
	return 0; // We never found a contiguous memory block
}


void releasepages(paddr_t paddr)
{
	int i, j;
	for (i = 0; coremap[i]->paddr != paddr; i++);
	
	assert(coremap[i]->block_len != -1);
	
	for (j = 0; j < coremap[i]->block_len; j++) {
		coremap[j]->used = 0;
	}
	
	coremap[i]->block_len = -1;
}

#endif
