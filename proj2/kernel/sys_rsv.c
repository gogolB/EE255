#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include "sys_rsv.h"


#define UINT64 long long int 
#define UINT32 long int 

static inline uint64_t lldiv(uint64_t dividend, uint32_t divisor)
{
    uint64_t __res = dividend;
    do_div(__res, divisor);
    return(__res);
}

static DEFINE_MUTEX(rsv_task_mutex);

typedef struct {
	pid_t pid;
	struct timespec T;
} tTuple; 


static tTuple rsv_tasks[50];

static int num_rsv_tasks = 0;

#define timer_c_granularity 10*1000*1000

bool C_lessthan_T(struct timespec *C, struct timespec *T)
{
	if(C->tv_sec == T->tv_sec)
		return C->tv_nsec < T->tv_nsec;
	else
		return C->tv_sec < T->tv_sec; 
}

void prune(void)
{
	return;
}

void update_scheduler(void)
{

	struct sched_param param;
	int status;
	int low_priority, high_priority;

	struct task_struct *task;
	struct pid *pid_struct;

	struct timespec *currentPeriod;
	int i;
	pid_t pid;
	
	high_priority = 80;

	low_priority = 30;

	prune();

	for(i = 0; i < num_rsv_tasks; i++)
	{	
		pid = rsv_tasks[i].pid;
		printk(KERN_ALERT"Fetching task for pid: %u\n", pid);
		// Get the TCB
		pid_struct = find_get_pid(pid);	
		task = (struct task_struct*)pid_task(pid_struct,PIDTYPE_PID);

		printk(KERN_ALERT"adjusting scheduling for task: %s\n", task->comm);	
		
		if(i == 0)
		{
			currentPeriod = &task->T;
		}
		
		// Get the old policy		
		task->old_policy = task->policy;
		if(task->old_policy < 0)
		{
			// Failed to get something. Skip this one.
			printk(KERN_ALERT"[RSV] Failed to get policy for pid: %u\n",pid);
			continue;
		}

		// Get the old priority
		task->old_priority = task->prio;

		// Set the new priority
		printk(KERN_ALERT"CURRENT: %ld,%ld\n",currentPeriod->tv_sec,currentPeriod->tv_nsec);
		printk(KERN_ALERT"T: %ld,%ld\n",task->T.tv_sec,task->T.tv_nsec);
		if(C_lessthan_T(currentPeriod,&task->T))
		{
			printk(KERN_ALERT"C<T\n");
			currentPeriod = &task->T;
			high_priority--;
		}

		param.sched_priority = high_priority;
		status = sched_setscheduler(task,SCHED_FIFO,&param);
		if(status == -1)
		{
			// Failed to set something. Skip this one.
			printk(KERN_ALERT"[RSV] Failed to set new sched and priority for pid: %u\n",pid);
			continue;
		}
		printk(KERN_ALERT"[RSV] Set new sched and priority (%d) for pid: %u\n",high_priority,pid);

		
	}
}

// Assume the list is already organized.
void addRsvTask(pid_t pid, struct timespec *T)
{
	int index = num_rsv_tasks;
	int i;
	mutex_lock(&rsv_task_mutex);
	// Find the index to insert at
	for(i = 0; i < num_rsv_tasks; i++)
	{
		if(C_lessthan_T(T, &rsv_tasks[i].T))
		{
			index = i;
			break;
		}
	}
	printk(KERN_ALERT"Index for new PID(%u) is %d\n",pid,index);

	// Shift everything down by one.
	for(i = index; i < num_rsv_tasks;i++)
	{
		rsv_tasks[i+1].pid = rsv_tasks[i].pid;
		rsv_tasks[i+1].T.tv_sec = rsv_tasks[i].T.tv_sec;
		rsv_tasks[i+1].T.tv_nsec = rsv_tasks[i].T.tv_nsec;
	}
	// Insert the new one.
	rsv_tasks[index].pid = pid;
	rsv_tasks[index].T.tv_sec = T->tv_sec;
	rsv_tasks[index].T.tv_nsec = T->tv_nsec;
	//printk(KERN_ALERT"Task inserted!\n");
	// Increment number of reserved tasks
	num_rsv_tasks++;

	// Update the scheduler.
	update_scheduler();
	mutex_unlock(&rsv_task_mutex);
}


// Assume the list is already organized. 
void removeRsvTask(pid_t pid)
{
	int index = -1;
	int i;
	mutex_lock(&rsv_task_mutex);
	// Find the index.
	for(i = 0; i < num_rsv_tasks; i++)
	{
		if(pid == rsv_tasks[i].pid)
		{
			index = i;
			break;
		}
	}

	if(index == -1)
	{
		printk(KERN_ALERT"[RSV] Failed to find pid: %u\n",pid);
		return;
	}

	
			
	// Shift everything up by one.
	for(i = index+1; i < num_rsv_tasks;i++)
	{
		rsv_tasks[i - 1].pid = rsv_tasks[i].pid;
		rsv_tasks[i - 1].T = rsv_tasks[i].T;
	}
	// Remove the task
	rsv_tasks[num_rsv_tasks].pid = 0;
	rsv_tasks[num_rsv_tasks].T.tv_sec = 0;
	rsv_tasks[num_rsv_tasks].T.tv_nsec = 0;
	// Reduce the number of reserved tasks
	num_rsv_tasks--;
	// Update the scheduler
	update_scheduler();
	printk(KERN_ALERT"Removed PID:%d\n",pid);
	mutex_unlock(&rsv_task_mutex);
}

enum hrtimer_restart hr_c_timer_callback(struct hrtimer * timer)
{
	// This regets the task
	struct task_struct * task;
	task = container_of(timer, struct task_struct, hr_C_Timer);


	task->currentC.tv_nsec += timer_c_granularity;
	//printk(KERN_ALERT"C TIMER reached for PID: %u\n", task->pid);
	if(task->accumulate != 1)
	{
		//printk(KERN_ALERT"C TIMER NON_ACUM RESTART for PID: %u\n", task->pid);
		hrtimer_forward_now(timer,ktime_set(0,timer_c_granularity));
		return HRTIMER_RESTART;
	}
	// Assuming branch predict true.
	if(task->currentC.tv_nsec < 1000*1000*1000)
	{

		hrtimer_forward_now(timer,ktime_set(0,timer_c_granularity));
		//printk(KERN_ALERT"C TIMER NORMAL RESTART for PID: %u\n", task->pid);
		return HRTIMER_RESTART;
	}

	// More than 1 second worth of Nano seconds have passed, so update all the variables.
	task->currentC.tv_sec += 1;
	task->currentC.tv_nsec -= 1000*1000*1000;
	hrtimer_forward_now(timer,ktime_set(0,timer_c_granularity));
	//printk(KERN_ALERT"C TIMER SECOND RESTART for PID: %u\n", task->pid);
	return HRTIMER_RESTART;

}

enum hrtimer_restart hr_t_timer_callback(struct hrtimer *timer)
{
	int ret;
	uint64_t t1, t2;
	char wasActive = 0;
	// This will reget the task.
	struct task_struct * task;
	task = container_of(timer, struct task_struct, hr_T_Timer);

	if(!task)
	{
		printk(KERN_ALERT"T TIMER ERROR\n");
		//hrtimer_forward_now(timer,ktime_set(task->T->tv_sec,task->T->tv_sec));
		return HRTIMER_RESTART;
	}


	//printk(KERN_INFO"T TIMER reached for PID: %u\n", task->pid);

	//spin_lock_irqsave(&task->sp_lock,flags);
	// Stop the other C timer.
	if(hrtimer_active(&task->hr_C_Timer))
	{
		ret = hrtimer_cancel(&task->hr_C_Timer);
		wasActive = 1;
	}
	// The task has overun its budget
	//printk(KERN_ALERT"T TIMER Checking C PID: %u\n", task->pid);
	//printk(KERN_ALERT"T TIMER C is %ld,%ld\n", task->C->tv_sec,task->C->tv_nsec);
	//printk(KERN_ALERT"T TIMER currentC is %ld,%ld\n", task->currentC.tv_sec,task->currentC.tv_nsec);
	if(C_lessthan_T(&task->C,&task->currentC))
	{
		// Convert to NS
		t1 = 1000*1000*1000*task->currentC.tv_sec + task->currentC.tv_nsec;
		//printk(KERN_ALERT"Current C is %ld,%ld\n", task->currentC.tv_sec,task->currentC.tv_nsec);
		
		// Convert to NS		
		t2 = 1000*1000*1000*task->T.tv_sec + task->T.tv_nsec;
	
		printk(KERN_ALERT"Task %s: budget overrun (util:%llu%%)\n", task->comm, lldiv(t1*100,t2));
		
		// Send the signal. This will kill the process by default. 
		kill_pid(task_pid(task), SIGUSR1, 1);
	}

	// Reset the Counter
	//printk(KERN_ALERT"T TIMER Clearing C PID: %u\n", task->pid);
	task->currentC.tv_sec = 0;
	task->currentC.tv_nsec = 0;

	// Restart the C timer.
	//hrtimer_forward_now(&task->hr_C_Timer->timer,ktime_set(0,timer_c_granularity));
	//printk(KERN_ALERT"T TIMER RESTARTING C PID: %u\n", task->pid);
	if(wasActive)
		hrtimer_restart(&task->hr_C_Timer);	
	
	// Wake up the task
	//printk(KERN_ALERT"Task %s with PID:%u\n", task->comm, task->pid);
	ret = wake_up_process(task);
	if(ret == 1)
		printk(KERN_INFO"Process %s was woken up\n", task->comm);
	else
		printk(KERN_INFO"Process %s was up\n", task->comm);

	//spin_unlock_irqrestore(&task->sp_lock,flags);
	//printk(KERN_ALERT"FORWARDING FOR %ld,%ld\n",task->T.tv_sec,task->T.tv_sec);
	hrtimer_forward_now(timer,ktime_set(task->T.tv_sec,task->T.tv_nsec));
	//printk(KERN_ALERT"Task %s: restarted, timer restarted\n", task->comm);
	return HRTIMER_RESTART;
}

asmlinkage int sys_set_rsv(pid_t pid, struct timespec *C, struct timespec *T)
{
	pid_t target_pid;
	struct task_struct *task;
	struct pid *pid_struct;

	ktime_t ktime_T;
	ktime_t ktime_C;

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
	
	printk(KERN_INFO"[RSV]Setting task to be reserved\n");
	task->rsv_task = 1;
	task->C.tv_sec = C->tv_sec;
	task->C.tv_nsec = C->tv_nsec;
	task->T.tv_sec = T->tv_sec;
	task->T.tv_nsec = T->tv_nsec;
	task->accumulate = 1;

	printk(KERN_INFO"[RSV]Setting up task timers for PID: %u\n", target_pid);
	
	ktime_C = ktime_set(0, timer_c_granularity); 	// Run every 10 miliseconds.
	ktime_T = ktime_set(T->tv_sec,T->tv_nsec);	// Run every T

	
	// Init the timer.
	hrtimer_init(&task->hr_C_Timer,CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_init(&task->hr_T_Timer,CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	
	// Set the call back functions
	task->hr_C_Timer.function = &hr_c_timer_callback;
	task->hr_T_Timer.function = &hr_t_timer_callback;

	// Start the timers
	hrtimer_start(&task->hr_C_Timer, ktime_C, HRTIMER_MODE_REL);
	hrtimer_start(&task->hr_T_Timer, ktime_T, HRTIMER_MODE_REL);
	printk(KERN_INFO"[RSV] Started task timers for PID: %u\n", target_pid);

	// Make RT
	addRsvTask(target_pid, &task->T);
	return 0;
}

asmlinkage int sys_cancel_rsv(pid_t pid)
{
	pid_t target_pid;
	struct task_struct *task;
	struct pid *pid_struct;
	int status;
	struct sched_param param;

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
		// Stop the timers
		hrtimer_cancel(&task->hr_C_Timer);
		hrtimer_cancel(&task->hr_T_Timer);
		
		// Clear the flag.
		task->rsv_task = 0;
		printk(KERN_INFO"[RSV] Rsv canceled for PID: %u\n",target_pid);

		// Remove it.
		removeRsvTask(target_pid);

		// Restore the old policy and priority.
		param.sched_priority = task->old_priority;
		status = sched_setscheduler(task, task->old_policy, &param);
		if(status <0)
		{
			printk(KERN_ALERT"[RSV] Failed to restore old params for canceling\n");
			return -1;
		}
		return 0;
	}
	else
	{
		// Task was not reserved, no need to do anything.
		printk(KERN_WARNING"[RSV] No Reservation for PID: %u\n",target_pid);
		return -1;
	}
}

