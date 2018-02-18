#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>

#include <time.h>
//#include <linux/ktime.h>
//#include <linux/hrtimer.h>

int main(void)
{
	int count = 1;
	struct timespec t_0;
	struct timespec t_now;
	struct timespec t_diff;
	clock_gettime(CLOCK_REALTIME, &t_0);
	while(1)
	{
		clock_gettime(CLOCK_REALTIME, &t_now);
		t_diff.tv_sec = t_now.tv_sec - t_0.tv_sec;
		t_diff.tv_nsec = t_now.tv_nsec - t_0.tv_nsec;
		printf("period %d: %d.%09d\n", count++, t_diff.tv_sec, t_diff.tv_nsec);
		syscall(399);
	}
	return 0;
}
