#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>

#include <time.h>
//#include <linux/ktime.h>
//#include <linux/hrtimer.h>

int main(void)
{
	int budget = 10;    // Arbitrary 'budget value' in nsecs
	
	int count = 1;
	struct timespec t_0;
	struct timespec t_now;
	struct timespec t_diff;
	clock_gettime(CLOCK_REALTIME, &t_0);
	while(1)
	{
		while(1)
		{
			clock_gettime(CLOCK_REALTIME, &t_now);
			t_diff.tv_sec = t_now.tv_sec - t_0.tv_sec;
			t_diff.tv_nsec = t_now.tv_nsec - t_0.tv_nsec;
			if (t_diff.tv_nsec > budget)
			{
				break;
			}
		}		
			
		printf("period %d: %d.%09d\n", count++, t_diff.tv_sec, t_diff.tv_nsec);
		syscall(399);
	}
	return 0;
}
