#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/pid.h>

bool C_lessthan_T(struct timespec *C, struct timespec *T)
{
	if(C->tv_sec == T->tv_sec)
		return C->tv_nsec <= T->tv_nsec;
	else
		return C->tv_sec <= T->tv_sec; 
}


asmlinkage int sys_set_rsv(pid_t pid, struct timespec *C, struct timespec *T)
{
	pid_t target_pid;
	struct task_struct *task;
	struct pid *pid_struct;

	// Check all the params for nulls
	if(C == 0 || T == 0)
	{
		// We got bad addresses for C or T
		printk(KERN_ALERT"[RSV]Recieved bad address for C(0x%p) or T(0x%p)",C, T);
		return -1;
	}

	// Make sure that C < T or C/T is <= 1
	if(!C_lessthan_T(C,T))
	{
		printk(KERN_ALERT"[RSV]C is not <= T");
		return -1;
	}
	
	// Set the target PID.
	if(pid == 0)
	{
		printk(KERN_INFO"[RSV]Getting PID of calling task");
		target_pid = current->pid;
	}
	else
	{
		target_pid = pid;
	}

	printk(KERN_INFO"[RSV]Target PID: %u", target_pid);
	

	// Now we get the TCB
	pid_struct = find_get_pid(pid);
	task = (struct task_struct*)pid_task(pid_struct,PIDTYPE_PID);		
	
	printk(KERN_INFO"[RSV]Setting task to be reserved");
	task->rsv_task = 1;
	task->C = *C;
	task->T = *T;

	return 0;
}

asmlinkage int sys_cancel_rsv(pid_t pid)
{
	return -1;
}

