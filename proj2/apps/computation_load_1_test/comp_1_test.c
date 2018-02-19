#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <time.h>
#include <stdbool.h>

#define MS_TO_NS 1000*1000

struct timespec timespec_diff(struct timespec *t1, struct timespec *t2)
{
	struct timespec result;
    	if ((t2->tv_nsec - t1->tv_nsec) < 0) 
	{
	        result.tv_sec = t2->tv_sec - t1->tv_sec - 1;
	        result.tv_nsec = t2->tv_nsec - t1->tv_nsec + 1000000000;
	} 
	else 
	{
	        result.tv_sec = t2->tv_sec - t1->tv_sec;
	        result.tv_nsec = t2->tv_nsec - t1->tv_nsec;
	}

    	return result;
}

bool C_lessthan_T(struct timespec *C, struct timespec *T)
{
	if(C->tv_sec == T->tv_sec)
		return C->tv_nsec <= T->tv_nsec;
	else
		return C->tv_sec <= T->tv_sec; 
}


int main(void)
{
	struct timespec C, T;
	struct timespec t1, t2, diff;
	int r;
	// Test Set C to zero;
	C.tv_sec = 0;
	C.tv_nsec = 20*MS_TO_NS;

	// Test Set T to zero;
	T.tv_sec = 0;
	T.tv_nsec = 100*MS_TO_NS;

	// Set Reservation
	r = syscall(397,0,&C,&T);
	while (1) {
		struct timespec t1, t2;
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1);
		while (1) 
		{
			clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);
			diff = timespec_diff(&t1,&t2);
			if (!C_lessthan_T(&diff , &C)) 
				break;
		}
		
		// Wait for next period
		r = syscall(399);
	}

	return 0;
}
