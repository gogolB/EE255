#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/mm_types.h>
#include <linux/mm.h>

asmlinkage int sys_show_vm_areas(int pid)
{
	struct task_struct *task;
	struct pid *pid_struct;
	pid_t target_pid;
	
	struct mm_struct *mmp;
	struct vm_area_struct *vmp;
	
	// Set the target PID.
	if(pid == 0)
	{
		printk(KERN_INFO"[SHOW_VM_AREA]Getting PID of calling task\n");
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
		printk(KERN_ALERT"[SHOW_VM_AREA] Failed to get a pid struct. Bad PID: %u\n",target_pid);
		return -1;
	}	
	
	task = (struct task_struct*)pid_task(pid_struct,PIDTYPE_PID);
	mmp = task->active_mm;
	printk(KERN_INFO"[Memory-maped areas of process %u]\n",target_pid);
	// Go through all the VM_Structs
	vmp = mmp->mmap;
	while(1)
	{
		// Check if it is locked
		if((vmp->vm_flags & VM_LOCKED) == VM_LOCKED)
		{
			printk(KERN_INFO"%#010lx - %#010lx: %lu bytes [L]\n",vmp->vm_start, vmp->vm_end, vmp->vm_end - vmp->vm_start);
		}
		else
		{
			printk(KERN_INFO"%#010lx - %#010lx: %lu bytes\n",vmp->vm_start, vmp->vm_end, vmp->vm_end - vmp->vm_start);
		}
			
		
		// Go to the next VMA		
		vmp = vmp->vm_next;
		if(vmp == NULL)
			break;
	
	}
	
	return 0;
}
