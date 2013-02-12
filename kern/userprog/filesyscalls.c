#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <uio.h>
#include <thread.h>
#include <curthread.h>
#include <vfs.h>
#include <vnode.h>
#include <syscall.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <synch.h>
#include "opt-A2.h"

#if OPT_A2

// Performs the equivalent of mk_kuio for user-level

static void mk_useruio(struct uio *u, userptr_t buf, size_t len, off_t offset, enum uio_rw rw)
{
  	assert(u);

 	u->uio_iovec.iov_ubase = buf;
  	u->uio_iovec.iov_len = len;
  	u->uio_offset = offset;
  	u->uio_resid = len;
  	u->uio_segflg = UIO_USERSPACE;
 	u->uio_rw = rw;
  	u->uio_space = curthread->t_vmspace;
}

// Opens a file through the file descriptor table

int sys_open(userptr_t filename, int flags, int mode, int *retval)
{
	(void)mode;

  	char fname[PATH_MAX];
  	int result;

  	result = copyinstr(filename, fname, sizeof(fname), NULL);
  	if (result) 
	{
    		return result;
  	}

 	return fdtable_open(fname, flags, retval);
}

// Reads part of a file and returns the amount left to be read for subsequent reads

int sys_read(int fd, userptr_t buf, size_t size, int *retval)
{
  	int result;
  	struct file *f;

	// Get the file descriptor entry corresponding to the file
  	result = fdtable_getentry(fd, &f);
  	if (result)
	{
    		return result;
  	}

  	struct uio readuio;

	// Create a uio structure to do the reading
  	mk_useruio(&readuio, buf, size, f->offset, UIO_READ);

	// Read from the file using the vnode
  	result = VOP_READ(f->file_vnode, &readuio);
  	if (result) 
	{
    		return result;
  	}

	// Update the offset
	f->offset = readuio.uio_offset;

	// Return the amount left to be read
 	*retval = size - readuio.uio_resid;

  	return 0;
}

// Writes a part of output to a file and returns the amount left to be written for subsequent writes

int sys_write(int fd, userptr_t buf, size_t size, int *retval)
{
  	int result;
  	struct file *f;

	// Get the file descriptor entry corresponding to the file
  	result = fdtable_getentry(fd, &f);
  	if (result)
	{
   		return result;
  	}

	struct uio writeuio;

	// Create a uio structure to do the writing
  	mk_useruio(&writeuio, buf, size, f->offset, UIO_WRITE);

	// Write to the file using the vnode
 	result = VOP_WRITE(f->file_vnode, &writeuio);
  	if (result) 
	{
    		return result;
  	}

	// Update the offset
	f->offset = writeuio.uio_offset;

	// Return the amount left to be written
 	*retval = size - writeuio.uio_resid;

  	return 0;
}

// Closes a file through the file descriptor table

int sys_close(int fd)
{
  	return fdtable_close(fd);
}

#endif /* OPT_A2 */
