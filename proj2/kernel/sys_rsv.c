#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/mutex.h>

#define UINT64 long long int 
#define UINT32 long int 

static inline uint64_t lldiv(uint64_t dividend, uint32_t divisor)
{
    uint64_t __res = dividend;
    do_div(__res, divisor);
    return(__res);
}

DEFINE_MUTEX(rsv_task_mutex);

typedef struct {
	pid_t pid;
	struct timespec *T;
} tTuple; 


static tTuple rsv_tasks[50];

static int num_rsv_tasks;

#define timer_c_granularity 10*1000*1000

bool C_lessthan_T(struct timespec *C, struct timespec *T)
{
	if(C->tv_sec == T->tv_sec)
		return C->tv_nsec <= T->tv_nsec;
	else
		return C->tv_sec <= T->tv_sec; 
}

void update_scheduler(void)
{
	struct sched_param param;
	int status;
	int low_priority, high_priority;

	struct task_struct *task;
	struct pid *pid_struct;

	struct timespec *currentPeriod;
	
	high_priority = sched_get_priority_max(SCHED_FIFO);
	if(high_priority < 0)
	{
		printk(KERN_ALERT"[RSV] Could not get high priority\n");
		return;
	}
	low_priority = sched_get_priority_min(SCHED_FIFO);
	if(low_priority < 0)
	{
		printk(KERN_ALERT"[RSV] Could not get low priority\n");
		return;
	}
		

	for(int i = 0; i < num_rsv_tasks; i++)
	{
		pid_t = rsv_tasks[i].pid;
		// Get the TCB
		pid_struct = find_get_pid(pid);	
		task = (struct task_struct*)pid_task(pid_struct,PIDTYPE_PID);	
		
		if(i == 0)
		{
			currentPeriod = task->T;
		}
		
		// Get the old policy		
		task->old_policy = sched_getscheduler(pid);
		if(task->old_policy < 0)
		{
			// Failed to get something. Skip this one.
			printk(KERN_ALERT"[RSV] Failed to get policy for pid: %u\n",pid);
			continue;
		}

		// Get the old priority
		status = sched_getparam(pid, &param);
		if(status < 0)
		{
			// Failed to get something. Skip this one.
			printk(KERN_ALERT"[RSV] Failed to get priority for pid: %u\n",pid);
			continue;
		}
		task->old_priority = param.sched_priority;

		// Set the new priority
		param.sched_priority = high_priority;
		status = sched_setcheduler(pid,SCHED_FIFO,&param);
		if(status < 0)
		{
			// Failed to set something. Skip this one.
			printk(KERN_ALERT"[RSV] Failed to set sched and priority for pid: %u\n",pid);
			continue;
		}

		if(C_lessthan_T(currentPeriod,task->T))
		{
			currentPeriod = task->T;
			high_priority--;
		}
		
	}
}

// Assume the list is already organized.
void addRsvTask(pid_t pid, struct timespec *T)
{
	mutex_lock(rsv_task_mutex);
	// Find the index to insert at
	int index;
	for(int i = 0; i < num_rsv_tasks; i++)
	{
		if(C_lessthan_T(T, &rsv_tasks[i].T))
		{
			index = i;
			break;
		}
	}

	// Shift everything down by one.
	for(int i = index; i < num_rsv_tasks;i++)
	{
		rsv_tasks[i + 1].pid = rsv_tasks[i].pid;
		rsv_tasks[i + 1].T = rsv_tasks[i].T;
	}
	// Insert the new one.
	rsv_tasks[index].pid = pid;
	rsv_task[index].T = T;

	// Increment number of reserved tasks
	num_rsv_taks++;

	// Update the scheduler.
	update_scheduler();
	mutex_unlock(rsv_task_mutex);
}


// Assume the list is already organized. 
void removeRsvTask(pid_t pid)
{
	mutex_lock(rsv_task_mutex);
	// Find the index.
	int index;
	for(int i = 0; i < num_rsv_tasks; i++)
	{
		if(pid == rsv_tasks[i].pid)
		{
			index = i;
			break;
		}
	}

	
			
	// Shift everything up by one.
	for(int i = index+1; i < num_rsv_tasks;i++)
	{
		rsv_tasks[i - 1].pid = rsv_tasks[i].pid;
		rsv_tasks[i - 1].T = rsv_tasks[i].T;
	}
	// Remove the task
	rsv_tasks[num_rsv_tasks].pid = 0;
	rsv_tasks[num_rsv_tasks].T = NULL;
	// Reduce the number of reserved tasks
	num_rsv_task++;
	// Update the scheduler
	update_scheduler();
	mutex_unlock(rsv_task_mutex);
}

enum hrtimer_restart hr_c_timer_callback(struct hrtimer * timer)
{
	// This regets the task
	struct task_struct *task;
	task = container_of(timer, struct task_struct, hr_C_Timer);

	task->currentC.tv_nsec += timer_c_granularity;
	
	// Assuming branch predict true.
	if(task->currentC.tv_nsec < 1000*1000*1000)
	{
		return HRTIMER_RESTART;
	}
	else
	{
		// More than 1 second worth of Nano seconds have passed, so update all the variables.
		task->currentC.tv_sec += 1;
		task->currentC.tv_nsec -= 1000*1000*1000;
		return HRTIMER_RESTART;
	}
}

enum hrtimer_restart hr_t_timer_callback(struct hrtimer *timer)
{
	int ret;
	uint64_t t1, t2;
	// This will reget the task.
	struct task_struct *task;
	task = container_of(timer, struct task_struct, hr_T_Timer);

	// Stop the other C timer.
	ret = hrtimer_cancel(&task->hr_C_Timer);

	// The task has overun its budget
	if(C_lessthan_T(task->C,&task->currentC))
	{
		t1 = 1000*1000*1000*task->currentC.tv_sec + task->currentC.tv_nsec;
		t2 = 1000*1000*1000*task->T->tv_sec + task->T->tv_nsec;
	
		printk("Task %s: budget overrun (util:%llu %%)\n", task->comm, lldiv(t1*100,t2));
		
		// Send the signal. This will kill the process be default. 
		kill_pid(task_pid(task), SIGUSR1, 1);
	}

	// Reset the Counter
	task->currentC.tv_sec = 0;
	task->currentC.tv_nsec = 0;

	// Restart the C timer.
	hrtimer_restart(&task->hr_C_Timer);	
	
	// Wake up the task
	kill_pid(task_pid(task), SIGCONT, 1);
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
	
	printk(KERN_INFO"[RSV]Setting task to be reserved\n");
	task->rsv_task = 1;
	task->C = C;
	task->T = T;

	printk(KERN_INFO"[RSV]Setting up task timers for PID: %u\n", target_pid);
	
	ktime_C = ktime_set(0, timer_c_granularity); 	// Run every 100 microseconds.
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

	// Adding to RT scheduling
	addRsvTask(target_pid,T);
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
	
	pid_struct = find_get_pid(target_pid);
	if(pid_struct == NULL)
	{
		printk(KERN_ALERT"[RSV] Failed to find a pid struct. Bad PID:%u\n",target_pid);
		return -1;
	}
	
	task = (struct task_struct *)pid_task(pid_struct, PIDTYPE_PID);

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
		status = sched_getparam(target_pid,&param);
		if(status <0)
		{
			printk(KERNALERT"[RSV] Failed to get old parms for canceling\n");
			return -1;
		}
		param.sched_priority = task->old_priority;
		status = sched_setscheduler(target_pid, task->old_policy, &param);
		if(status <0)
		{
			printk(KERNALERT"[RSV] Failed to restore old params for canceling\n");
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

