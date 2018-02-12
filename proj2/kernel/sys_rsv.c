#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>

#define timer_c_granularity 100*1000

bool C_lessthan_T(struct timespec *C, struct timespec *T)
{
	if(C->tv_sec == T->tv_sec)
		return C->tv_nsec <= T->tv_nsec;
	else
		return C->tv_sec <= T->tv_sec; 
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
	// This will reget the task.
	struct task_struct *task;
	task = container_of(timer, struct task_struct, hr_T_Timer);

	// Stop the other C timer.
	ret = hrtimer_cancel(&task->hr_C_Timer);

	// Reset the Counter
	task->currentC.tv_sec = 0;
	task->currentC.tv_nsec = 0;

	// Restart the C timer.
	hrtimer_restart(&task->hr_C_Timer);	
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
	if(C == 0 || T == 0)
	{
		// We got bad addresses for C or T
		printk(KERN_ALERT"[RSV]Recieved bad address for C(0x%p) or T(0x%p)\n",C, T);
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
	pid_struct = find_get_pid(pid);
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
	
	pid_struct = find_get_pid(pid);
	task = (struct task_struct *)pid_task(pid_struct, PIDTYPE_PID);

	if(task->rsv_task == 1)
	{
		// Stop the timers
		hrtimer_cancel(&task->hr_C_Timer);
		hrtimer_cancel(&task->hr_T_Timer);
		
		// Clear the flag.
		task->rsv_task = 0;
		printk(KERN_INFO"[RSV] Rsv canceled for PID: %u\n",target_pid);
		return 0;
	}
	else
	{
		// Task was not reserved, no need to do anything.
		printk(KERN_WARN"[RSV] No Reservation for PID: %u\n",target_pid);
		return -1;
	}
}

