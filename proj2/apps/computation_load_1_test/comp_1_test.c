#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

static volatile int keepRunning = 1;
void intHandler(int dummy)
{
	keepRunning = 0;
}
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

void overflowHandler()
{
	printf("I USED TOO MUCH TIME!\n");
	//exit(-1);
}

int main(void)
{
	struct timespec C, T;
	struct timespec t1, t2, diff;
	struct timespec req,rem;
	
	// Register sig int handler.
	signal(SIGINT, intHandler);
	signal(SIGUSR1, overflowHandler);	

	req.tv_sec = 0;
	req.tv_nsec = 80*MS_TO_NS;
	int r;
	// Test Set C to zero;
	C.tv_sec = 0;
	C.tv_nsec = 20*MS_TO_NS;

	// Test Set T to zero;
	T.tv_sec = 0;
	T.tv_nsec = 100*MS_TO_NS;

	// Set Reservation
	r = syscall(397,0,&C,&T);
	printf("Reservation set! r = %d\n", r);
	
	if(r == -1)
	{
		printf("Failed to set reservation\n");
		return -1;
	}
	
	while (keepRunning) {
		struct timespec t1, t2;
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1);
		while (1) 
		{
			clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2);
			diff = timespec_diff(&t1,&t2);
			if (!C_lessthan_T(&diff,&C)) 
				break;
			printf("Diff: %d,%d\n",diff.tv_sec, diff.tv_nsec);
		}
		// Wait for next period
		r = syscall(399);
		printf("value of r = %d\n", r);
	}

	return 0;
}
