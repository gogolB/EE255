#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pid.h>


asmlinkage int sys_show_segment_info(int pid)
{
	return -1;
}
