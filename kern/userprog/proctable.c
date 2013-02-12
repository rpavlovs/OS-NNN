#include "opt-A2.h"

#include <proctable.h>
#include <kern/errno.h>
#include <lib.h>

#if OPT_A2

// The process table
// An array that contains pointers to processes, where the index is the pid of the process.
static struct process **proctable;

// "Private" helper functions
void decouple_parent_from_children(struct process *parent)
{
	int i;
	for (i = PROC_MIN; i < PROC_MAX; i++) {
		if (proctable[i] != NULL && proctable[i]->parent != NULL && i != parent->pid) {
			struct process *child = proctable[i];
			if (child->parent->pid == parent->pid) {
				child->parent = NULL;
			}
		}
	}
}

// *Public" header functions
void clean_exited_processes()
{
	int i;
	int exited = STATUS_EXITED;
	for (i = PROC_MIN; i < PROC_MAX; i++) {
		if (proctable[i] != NULL && proctable[i]->status == exited) {
			// This process is a candidate for deletion since it has exited
			if (proctable[i]->parent == NULL || proctable[i]->parent->status == exited) {
				// The parent has either finished running, or has exited 
				// Either way, it cannot request waitpid from its child, so remove it
				proctable_remove_process(proctable[i]->pid);
			}
		}
	}
}

// Initialize the process table
void proctable_bootstrap()
{
	proctable = kmalloc(sizeof(struct process*) * PROC_MAX); // This is an array of pointers to struct processes
	proctable_lock = lock_create("proctable lock");

	int i;
	for (i = PROC_MIN; i < PROC_MAX; i++) {
		proctable[i] = NULL;
	}
}

// Add a new process with no parent
struct process *proctable_add_root_process() 
{
	// Before we add a new process, we should try and clean up the old processes
	clean_exited_processes();

	// Allocate a new pid
	pid_t pid = 0;
	int i;
	for (i = PROC_MIN; pid < PROC_MAX; i++) {
		if (proctable[i] == NULL) {
			pid = i;
			break;
		}
	}

	// Return an error if all pids are taken
	if (pid == 0) return NULL;

	// We have a pid, so we create a new process that will match it
	struct process *new_process = kmalloc(sizeof(struct process));
	new_process->pid = pid;
	new_process->status = STATUS_RUNNING;
	new_process->parent = NULL;
	new_process->t_fdtable = NULL;
	new_process->waitpid_cv = cv_create("waitpid cv");
	new_process->t_fdtable_lock = lock_create("file table lock");

	proctable[pid] = new_process;
	return new_process;
}

// Add a new process with a parent
// This is called from fork
struct process *proctable_add_child_process(struct process *parent) 
{
	// Before we add a new process, we should try and clean up the old processes
	clean_exited_processes();

	// Allocate a new pid
	pid_t pid = 0;
	int i;
	for (i = PROC_MIN; pid < PROC_MAX; i++) {
		if (proctable[i] == NULL) {
			pid = i;
			break;
		}
	}

	// Fail if no PIDs were found
	if (pid == 0) return NULL;

	// We have a pid, so we create a new process that will match it
	struct process *new_process = kmalloc(sizeof(struct process));
	new_process->pid = pid;
	new_process->status = STATUS_RUNNING;
	new_process->parent = parent;
	new_process->t_fdtable = NULL;
	new_process->waitpid_cv = cv_create("waitpid cv");
	new_process->t_fdtable_lock = lock_create("file table lock");

	proctable[pid] = new_process;
	return new_process;
}

// Remove the process from the process table entirely
int proctable_remove_process(pid_t pid)
{
	if (!(pid >= PROC_MIN && pid <= PROC_MAX) || proctable[pid] == NULL) {
		return EINVAL;
	}

	struct process *process_to_kill = proctable[pid];

	// Deallocate all memory that was allocated with kmalloc
	lock_destroy(process_to_kill->t_fdtable_lock);
	kfree(process_to_kill);

	// Release the PID
	proctable[pid] = NULL;
	return 0;
}

// Set the process as "exited", but leave it in memory
int proctable_set_process_exited(pid_t pid, int exitcode)
{
	if (!(pid >= PROC_MIN && pid <= PROC_MAX) || proctable[pid] == NULL) {
		return EINVAL;
	}

	struct process *process_to_exit = proctable[pid];
	
	process_to_exit->exit_code = exitcode;
	process_to_exit->status = STATUS_EXITED;
	decouple_parent_from_children(process_to_exit);
	return 0;
}

// Retrieve a process by its PID
// Return NULL if process does not exist
struct process* proctable_get_process(pid_t pid)
{
	if (!(pid >= PROC_MIN && pid <= PROC_MAX)) {
		return NULL;
	}

	return proctable[pid]; // This returns either NULL (if there is no process) or the pointer to the process itself
}

#endif /* OPT_A2 */
