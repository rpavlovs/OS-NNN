#include "opt-A2.h"

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
#if OPT_A2
int sys_open(userptr_t filename, int flags, int mode, int *retval);
int sys_read(int fd, userptr_t buf, size_t size, int *retval);
int sys_write(int fd, userptr_t buf, size_t size, int *retval);
int sys_close(int fd);
int sys_getpid(pid_t *retpid);
int sys_fork(struct trapframe *tf, pid_t *retpid);
int sys_waitpid(pid_t pid, u_int32_t *retstatus, int options, pid_t *retpid);
int sys__exit(int exitcode);
int sys_execv(userptr_t progname, userptr_t args);
#endif /* OPT_A2 */

#endif /* _SYSCALL_H_ */
