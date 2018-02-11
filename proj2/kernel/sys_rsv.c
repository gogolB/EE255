#include <linux/kernel.h>

asmlinkage int set_rsv(pid_t pid, struct timespec *C, struct timespect *T)
{
	return -1;
}

asmlinkage int cancel_rsv(pid_t pid)
{
	return -1;
}
