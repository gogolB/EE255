#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>

asmlinkage int sys_wait_until_next_period(void)
{
	printk(KERN_INFO"[WAIT_FOR_NEXT_PERIOD] PID: %u\n",current->pid);
	
	if(current->rsv_task != 1)
	{
		printk(KERN_ALERT"[WAIT_FOR_NEXT_PERIOD] Can not sleep, task not reserved\n");
		return -1;
	}
	printk(KERN_INFO"[WAIT_FOR_NEXT_PERIOD] Sleeping PID: %u\n",current->pid);
	printk(KERN_INFO"[WAIT_FOR_NEXT_PERIOD] Task is sleeping interruptable\n");
	set_current_state(TASK_PARKED);
	schedule();
	return 0;
}
