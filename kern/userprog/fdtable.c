#include <types.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/unistd.h>
#include <lib.h>
#include <synch.h>
#include <uio.h>
#include <thread.h>
#include <curthread.h>
#include <vfs.h>
#include <vnode.h>
#include <syscall.h>
#include "opt-A2.h"

#if OPT_A2

// Helper function to return the next available file descriptor

int assign_fd()
{
	struct fdtable *table = curthread->t_process->t_fdtable;
	
	int fd;
	for (fd = 0; fd < OPEN_MAX; fd++)
	{
		if (table->entries[fd] == NULL)
		{
			return fd;
		}
	}

	return fd;
}

// Opens a file and adds it to the table

int fdtable_open(char *name, int flags, int *retfd)
{
	// Get the next available file descriptor
  	int fd = assign_fd();
  	if (fd == OPEN_MAX)
	{
    		return EMFILE;
  	}

	// Allocate the entry structure
  	struct file *f = kmalloc(sizeof(struct file));
  	if (f == NULL)
	{
    		return ENOMEM;
  	}

	// Initialize the structure
	f->accesstype = flags;
	f->offset = 0;
  	f->refcount = 1;

	// Open the file for the corresponding vnode
  	int result = vfs_open(name, flags, &(f->file_vnode));
  	if (result)
	{
    		kfree(f->file_vnode);
    		kfree(f);
    		return result;
  	}

	// Set the table entry
  	curthread->t_process->t_fdtable->entries[fd] = f;

  	*retfd = fd;
  	return 0;
}

// Closes a file and removes it from the table

int fdtable_close(int fd)
{
  	struct file *f;

	// Get the entry from the table
	int result = fdtable_getentry(fd, &f);
  	if (result)
	{
    		return result;
  	}

	// Decrement its' reference count
  	f->refcount--;

	// If it is no longer needed
	// 1) close the corresponding vnode
	// 2) deallocate the table entry and reset it
  	if (f->refcount == 0)
	{
    		vfs_close(f->file_vnode);
    		kfree(f);
    		curthread->t_process->t_fdtable->entries[fd] = NULL;
  	}

  	return 0;
}

// Initializes the table for a new process

int fdtable_create()
{
	// Allocate the table structure
	struct fdtable *table = kmalloc(sizeof(struct fdtable));
	if (table == NULL)
	{
		return ENOMEM;
	}

	int i;

	// Initialize the table entries to NULL
	// (in case any of the addresses contains garbage) 
	for (i = 0; i < OPEN_MAX; i++)
	{
		table->entries[i] = NULL;
	}

	// Assign the filetable
	curthread->t_process->t_fdtable = table;

	int result;
	int fd;
	char path[5];

	// Create the standard file descriptors
	// stdin, stdout and stderr consecutively

	strcpy(path, "con:");
  	result = fdtable_open(path, O_RDWR, &fd);
  	if (result)
	{
    		return result;
  	}

	strcpy(path, "con:");
  	result = fdtable_open(path, O_RDWR, &fd);
  	if (result)
	{
    		return result;
  	}
	
	strcpy(path, "con:");
  	result = fdtable_open(path, O_RDWR, &fd);
  	if (result)
	{
    		return result;
  	}

  	return 0;
}

// Returns an entry if it exists in the table

int fdtable_getentry(int fd, struct file **retfile)
{
	struct fdtable *table = curthread->t_process->t_fdtable;

	if (fd < 0 || fd >= OPEN_MAX)
	{
    		return EBADF;
  	}

  	if (table->entries[fd] == NULL)
	{
  		return EBADF;
  	}

  	*retfile = table->entries[fd];
  	return 0;
}

// Destroys the table after a process has completed

void fdtable_destroy()
{
	struct fdtable *table = curthread->t_process->t_fdtable;

	if (table == NULL)
	{
		return;
	}

	// If there are any entries remaining, close them
	int i;
	for (i = 0; i < OPEN_MAX; i++)
	{
		if (table->entries[i] != NULL)
		{
			fdtable_close(i);
		}
	}

	// Deallocate the table
	kfree(table);
}

#endif /* OPT_A2 */
