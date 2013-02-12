#include <kern/limits.h>
#include <kern/types.h>
#include <vnode.h>
#include "opt-A2.h"

#if OPT_A2

// File descriptor table for a process

#define OPEN_MAX 16 // maximum amount of file descriptors

// Table entries

struct file
{
	struct vnode *file_vnode;
	off_t offset;
	int accesstype;
	int refcount;
};

// The table itself

struct fdtable
{
	struct file *entries[OPEN_MAX];
};

// Opens a file and adds it to the table
int fdtable_open(char *name, int flags, int *retfd);

// Closes a file and removes it from the table
int fdtable_close(int fd);

// Initializes the table for a new process
int fdtable_create();

// Returns an entry if it exists in the table
int fdtable_getentry(int fd, struct file **retfile);

// Destroys the table after a process has finished
void fdtable_destroy();

#endif /* OPT_A2 */
