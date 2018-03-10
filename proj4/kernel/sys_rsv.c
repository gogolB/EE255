#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/init.h>
#include <linux/cpumask.h>

#include "sys_rsv.h"


#define UINT64 long long int 
#define UINT32 long int 

static inline uint64_t lldiv(uint64_t dividend, uint32_t divisor)
{
    uint64_t __res = dividend;
    do_div(__res, divisor);
    return(__res);
}

bool C_lessthan_T(struct timespec *C, struct timespec *T)
{
	if(C->tv_sec == T->tv_sec)
		return C->tv_nsec < T->tv_nsec;
	else
		return C->tv_sec < T->tv_sec; 
}


asmlinkage int sys_set_rsv(pid_t pid, struct timespec *C, struct timespec *T, int cpuid)
{
	pid_t target_pid;
	struct task_struct *task;
	struct pid *pid_struct;
	cpumask_t cpu_set;
	int returnValue;

	// Check all the params for nulls
	if(C == NULL || T == NULL)
	{
		// We got bad addresses for C or T
		printk(KERN_ALERT"[RSV]Recieved bad address for C(0x%p) or T(0x%p)\n",C, T);
		return -1;
	}
	
	// Check and make sure that C isn't zero
	if(C->tv_sec == 0 && C->tv_nsec == 0)
	{
		printk(KERN_ALERT"[RSV]C is 0!\n");
		return -1;
	}

	// Check and make sure that T isn't zero
	if(T->tv_sec == 0 && T->tv_nsec == 0)
	{
		printk(KERN_ALERT"[RSV]T is 0!\n");
		return -1;
	}
		
	// Make sure that C < T or C/T is <= 1
	if(!C_lessthan_T(C,T))
	{
		printk(KERN_ALERT"[RSV]C is not <= T\n");
		return -1;
	}

	if(cpuid < 0 || cpuid > 3)
	{
		printk(KERN_ALERT"[RSV] CPUID %d is not a valid CPUID. Please pick a CPUID between 0 and 3\n", cpuid);
		return -1;
	}


	// Set the target PID.
	if(pid == 0)
	{
		printk(KERN_INFO"[RSV]Getting PID of calling task\n");
		target_pid = current->pid;
	}
	else
	{
		target_pid = pid;
	}

	printk(KERN_INFO"[RSV]Target PID: %u\n", target_pid);
	

	// Now we get the TCB
	pid_struct = find_get_pid(target_pid);
	if(pid_struct == NULL)
	{
		printk(KERN_ALERT"[RSV] failed to get a pid struct. Bad PID: %u\n",target_pid);
		return -1;
	}	
	
	task = (struct task_struct*)pid_task(pid_struct,PIDTYPE_PID);

	
	if(task->rsv_task == 1)
	{
		// Can not reserve a task that is already reserved.
		printk(KERN_ALERT"[RSV] This task is already reserved. PID: %u\n",target_pid);
		return -1;
	}

	
	// After this point we can assume that the task is good and can be reserved.		
	
	printk(KERN_INFO"[RSV]Setting task to be reserved\n");
	// Set the flag for reservation
	task->rsv_task = 1;
	
	// Set all the C and T Values.
	task->C.tv_sec = C->tv_sec;
	task->C.tv_nsec = C->tv_nsec;
	task->T.tv_sec = T->tv_sec;
	task->T.tv_nsec = T->tv_nsec;

	// Set the CPU Affinity
	cpumask_clear(&cpu_set);
	cpumask_set_cpu(cpuid, &cpu_set);
	returnValue = sched_setaffinity(target_pid, &cpu_set);
	if(returnValue != 0)
	{
		printk(KERN_ALERT"[RSV] Failed to set Affinity. Error code: %d\n", returnValue);
		return -1;
	}
	return 0;
}

asmlinkage int sys_cancel_rsv(pid_t pid)
{
	pid_t target_pid;
	struct task_struct *task;
	struct pid *pid_struct;

	if(pid == 0)
	{
		target_pid = current->pid;
	}
	else
	{
		target_pid = pid;
	}

	
	// Find the TCB
	pid_struct = find_get_pid(target_pid);
	if(pid_struct == NULL)
	{
		printk(KERN_ALERT"[RSV] Failed to find a pid struct. Bad PID:%u\n",target_pid);
		return -1;
	}
	
	task = (struct task_struct *)pid_task(pid_struct, PIDTYPE_PID);

	// Check if it is reserved.
	if(task->rsv_task == 1)
	{
		// Clear the flag.
		task->rsv_task = 0;
		printk(KERN_INFO"[RSV] Rsv canceled for PID: %u\n",target_pid);
		return 0;
	}
	else
	{
		// Task was not reserved, no need to do anything.
		printk(KERN_WARNING"[RSV] No Reservation for PID: %u\n",target_pid);
		return -1;
	}
}

