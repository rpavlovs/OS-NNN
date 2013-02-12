/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>

#include "opt-A2.h"

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char **argv, int argc)
{
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}

	#if OPT_A2
	// Initialize the Process
	
	lock_acquire(proctable_lock);
	struct process *new_process = proctable_add_root_process();
	if (new_process == NULL) {
		return EAGAIN; // Error occured
	} else {
		curthread->t_process = new_process;
	}

	// Initialize file table
	result = fdtable_create();
	if (result) 
	{
		return result; // Error occured
	}

	lock_release(proctable_lock);
	
	// Argument Passing

	int i;
	size_t len;
	size_t stackoffset = 0;
	vaddr_t argvptr[argc+1];

	// Copy each argument onto the stack going downwards
	for (i = 0; i < argc; i++)
	{
		len = strlen(argv[i]) + 1; // add 1 for the terminating NULL
		stackoffset += len;
		argvptr[i] = stackptr - stackoffset;
		copyout(argv[i], (userptr_t) argvptr[i], len);
	}
	argvptr[argc] = 0; // terminating NULL pointer for array

	// Create space for the argument pointers
	stackoffset += sizeof(vaddr_t) * (argc+1);
	
	// Adjust the stack pointer and align it
	stackptr = stackptr - stackoffset - ((stackptr - stackoffset)%8);

	// Copy the argument pointers onto the stack
	copyout(argvptr, (userptr_t) stackptr, sizeof(vaddr_t) * (argc+1));
	
	// Enter user mode with the arguments in place
	md_usermode(argc, (userptr_t) stackptr, stackptr, entrypoint);
	#else
	/* Warp to user mode. */
	md_usermode(0 /*argc*/, NULL /*userspace addr of argv*/,
		    stackptr, entrypoint);
	#endif /* OPT_A2 */
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}

