#include <types.h>
#include <synch.h>
#include <fdtable.h>

#include "opt-A2.h"

#define STATUS_EXITED 0;
#define STATUS_RUNNING 1;
#define PROC_MIN 1
#define PROC_MAX 128

#if OPT_A2

// The concept of a process (part of a thread in OS/161)
struct process
{
    pid_t pid;

    struct process *parent;
    
	int status;
	int exit_code;

	struct fdtable *t_fdtable;
    struct lock *t_fdtable_lock;
    struct cv *waitpid_cv;
};

// Lock for the entire process table
struct lock *proctable_lock;

// Initialize the process table
void proctable_bootstrap();

// Add a process to the process table that has no parent (used for new processes)
struct process *proctable_add_root_process();

// Add a process to the process table that has a parent (used for fork)
struct process *proctable_add_child_process(struct process *parent);

// Remove a process entirely 
int proctable_remove_process(pid_t pid);

// Set the process to exited status, but don't remove it because other processes may need to know its exit code
int proctable_set_process_exited(pid_t pid, int exitcode);

// Retrieve a process by its PID
// Return NULL if process does not exist
struct process* proctable_get_process(pid_t pid);

#endif /* OPT_A2 */
