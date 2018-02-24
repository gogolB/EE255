#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/mm_types.h>


asmlinkage int sys_show_segment_info(int pid)
{
	struct task_struct *task;
	struct pid *pid_struct;
	pid_t target_pid;
	
	struct mm_struct *mmp;

	// Set the target PID.
	if(pid == 0)
	{
		printk(KERN_INFO"[SHOW_SEGMENT]Getting PID of calling task\n");
		target_pid = current->pid;
	}
	else
	{
		target_pid = pid;
	}
	

	// Now we get the TCB
	pid_struct = find_get_pid(target_pid);
	if(pid_struct == NULL)
	{
		printk(KERN_ALERT"[SHOW_SEGMENT] Failed to get a pid struct. Bad PID: %u\n",target_pid);
		return -1;
	}	
	
	task = (struct task_struct*)pid_task(pid_struct,PIDTYPE_PID);
	
	mmp = task->mm;

	printk(KERN_INFO"[Memory segment addresses of process %u]\n",target_pid);
	printk(KERN_INFO"%#010lx - %#010lx: code segment\n",mmp->start_code, mmp->end_code);
	printk(KERN_INFO"%#010lx - %#010lx: data segment\n",mmp->start_data, mmp->end_data);
	printk(KERN_INFO"%#010lx - %#010lx: heap segment\n",mmp->start_brk, mmp->brk);

	return 0;
}
