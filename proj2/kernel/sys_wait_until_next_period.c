#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>

struct ST{
	struct hrtimer HRT;
	struct task_struct *task;
}storage;

enum hrtimer_restart timer_callback( struct hrtimer *timer_for_restart )
{
	struct ST *s;
	int ret;
	printk(KERN_ALERT"[WAIT_FOR_NEXT_PERIOD] WOKE UP\n");
	//wake_up_process(task)

	// This will reget the task.
	s = container_of(timer_for_restart, struct ST, HRT);
	printk(KERN_ALERT"[WAIT_FOR_NEXT_PERIOD] Woking up process %s, PID: %d\n", s->task->comm, s->task->pid);
	ret = wake_up_process(storage.task);
	if(ret == 1)
	{
		printk(KERN_ALERT"[WAIT_FOR_NEXT_PERIOD] Task was woken UP\n");
	}
	else
	{
		printk(KERN_ALERT"[WAIT_FOR_NEXT_PERIOD] Task was UP\n");
	}
	return HRTIMER_NORESTART;
}

asmlinkage int sys_wait_until_next_period(void)
{
	ktime_t time;
	struct timespec ts;
	
	printk(KERN_INFO"[WAIT_FOR_NEXT_PERIOD] PID: %u\n",current->pid);
	
	if(current->rsv_task != 1)
	{
		printk(KERN_ALERT"[WAIT_FOR_NEXT_PERIOD] Can not sleep, task not reserved\n");
		return -1;
	}

	printk(KERN_INFO"[WAIT_FOR_NEXT_PERIOD] Sleeping PID: %u\n",current->pid);
	set_current_state(TASK_INTERRUPTIBLE);	
	printk(KERN_INFO"[WAIT_FOR_NEXT_PERIOD] Task is interruptable\n");
	
	time  = hrtimer_get_remaining(&current->hr_T_Timer->timer);
	ts = ktime_to_timespec(time);
	printk(KERN_INFO"[WAIT_FOR_NEXT_PERIOD] Time left on timer: %ld,%ld\n",ts.tv_sec, ts.tv_nsec);


	//printk(KERN_INFO"[WAIT_FOR_NEXT_PERIOD] Timer active: %d\n",hrtimer_active(&current->hr_T_Timer->timer));
	//printk(KERN_INFO"[WAIT_FOR_NEXT_PERIOD] Timer is queued: %d\n",hrtimer_is_queued(&current->hr_T_Timer->timer));
	//printk(KERN_INFO"[WAIT_FOR_NEXT_PERIOD] Timer is canceled\n");
	//hrtimer_cancel(&current->hr_T_Timer->timer);
	//hrtimer_forward(&current->hr_T_Timer,hrtimer_cb_get_time(&current->hr_T_Timer),ktime_set(current->T->tv_sec,current->T->tv_nsec));
	//hrtimer_start(&current->hr_T_Timer->timer, time, HRTIMER_MODE_REL);
	//printk(KERN_INFO"[WAIT_FOR_NEXT_PERIOD] Timer is restarted\n");
	//hrtimer_init( &storage.HRT, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	//storage.task = current;
	//storage.HRT.function = &timer_callback;
	//hrtimer_start(&storage.HRT, time, HRTIMER_MODE_REL);
	
	schedule();
	return 0;
}
