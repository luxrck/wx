#include <assert.h>

#include <kernel/kernel.h>
#include <kernel/proc.h>
#include <kernel/kshell.h>

// Choose a user environment to run and run it.
void scheduler(void)
{
	if (!tasks) goto ksh;
	struct proc *it = tasks;
	printf("sched ");
	while (it) {
		printf("[%d %d %d] ", it->pid, it->state, it->parent ? it->parent->pid : 0);
		it = it->next;
	}
	if (!ctask) ctask = tasks;
	struct proc *tk = ctask->next;
	while (tk != ctask) {
		if (!tk) {
			tk = tasks;
			continue;
		}
		if (tk->state == PROC_RUNNABLE)
			procrestore(tk);
		tk = tk->next;
	}
	if (ctask->state == PROC_RUNNING || ctask->state == PROC_RUNNABLE)
		procrestore(ctask);
	else
		procfree(ctask);
ksh:
	kshell();
	panic("scheduler never should reach here. ctask->state: %x", ctask->state);
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// LAB 4: Your code here.
}