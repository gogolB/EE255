#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <time.h>
#include <pthread.h>

#define MS_TO_NS 1000*1000
#define S_TO_NS 1000*1000*1000

void printTestHeader(const char* msg)
{
	printf("--------------------------------\n");
	printf("\t%s\n",msg);
	printf("--------------------------------\n");
}




int main(void)
{
	// We'll be testing with values from Lecture 5, slide 28
	//struct timespec for a few tasks we want to launch
	struct timespec C1;
	struct timespec T1;
	// Don't forget the threads!
	struct timespec C_thread1;
	struct timespec T_thread1;
	
	

	int r1;
	int r_thread;
	
	// Test Set C1 to 20ms;
	C1.tv_sec = 0;  // C1 = 20ms
	C1.tv_nsec = 0.02*1000*1000*1000;

	// Test Set T1 to 100ms;
	T1.tv_sec = 0;  // T1 = 100ms
	T1.tv_nsec = 0.1*1000*1000*1000;
	///////////////////////////////////////////////////////////////
	// Test Set C2 to 40ms;
	C_thread1.tv_sec = 0;  // C2 = 40ms
	C_thread1.tv_nsec = 0.04*1000*1000*1000;

	// Test Set T2 to 150ms;
	T_thread1.tv_sec = 0;  // T1 = 150ms
	T_thread1.tv_nsec = 0.15*1000*1000*1000;
	
    printTestHeader("Checking the test parameters");
    printf("C1_nsec = %.9ld\n", C1.tv_nsec);
    printf("T1_nsec = %.9ld\n", T1.tv_nsec);
    if (T1.tv_nsec != 100000000 || C1.tv_nsec != 20000000)
    {
		return -1;
	}
	printf("C_thread1_nsec = %.9ld\n", C1.tv_nsec);
    printf("T_thread1_nsec = %.9ld\n", T1.tv_nsec);
    if (T_thread1.tv_nsec != 150000000 || C_thread1.tv_nsec != 40000000)
    {
		return -1;
	}
	// Canceling a call that doesn't exist.
	printTestHeader("Canceling a call that doesn't exist");
	r1 = syscall(398,0);
	if(r1 != -1)
	{
		printf("Test didn't fail. Something went wrong\n");
		return -1;
	}
	printf("Canceling a non existing sys call worked!\n");

	
	// Valid C1 and T1
	printTestHeader("Valid C1 and T1 adresses");
	r1 = syscall(397,0,&C1,&T1);
	if(r1 != 0)
	{
		printf("Test failed, Something went wrong\n");
		return -1;
	}
	printf("Valid C1 and T1 passed\n");
	
	// Create thread and launch new reserve with C_thread1 and T_thread1
	
	 Attempt to cancel taksk1
	printTestHeader("Canceling task 1");
	r1 = syscall(398,0);
	if(r1 != 0)
	{
		printf("Test failed. Something went wrong\n");
		return -1;
	}
	printf("Canceling an existing task worked!\n");
	


	
	return 0;
}
