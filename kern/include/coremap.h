#include <types.h>
#include <vm.h>
#include "opt-A3.h"
#if OPT_A3

// Structs
struct coremap_entry {
	paddr_t paddr;
	int used;
	int block_len;
};

static struct coremap_entry **coremap;
static int coremap_size;
static volatile int coremap_ready = 0;

// Methods
void initialize_coremap();
paddr_t getppages(unsigned long npages);
void releasepages(paddr_t paddr);

#endif
