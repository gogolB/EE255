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
	struct timespec C2;
	struct timespec T2;
	// Don't forget the threads!
	struct timespec C_thread1;
	struct timespec T_thread1;
	
	

	int r1;
	int r2;
	int r_thread;
	
	// Test Set C1 to 20ms;
	C1.tv_sec = 0.02;  // C1 = 20ms
	C1.tv_nsec = C1.tv_sec*S_TO_NS;

	// Test Set T1 to 100ms;
	T1.tv_sec = 0.1;  // T1 = 100ms
	T1.tv_nsec = T1.tv_sec*S_TO_NS;
	///////////////////////////////////////////////////////////////
	// Test Set C2 to 40ms;
	C2.tv_sec = 0.04;  // C2 = 40ms
	C2.tv_nsec = C2.tv_sec*S_TO_NS;

	// Test Set T2 to 150ms;
	T2.tv_sec = 0.15;  // T1 = 150ms
	T2.tv_nsec = T2.tv_sec*S_TO_NS;
	///////////////////////////////////////////////////////////////
	// Let Task 3 be the thread


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
	
	// Valid C2 and T2
	printTestHeader("Valid C2 and T2 addresses");
	r2 = syscall(397,0,&C2,&T2);
	if(r2 != 0)
	{
		printf("Test failed, Something went wrong\n");
		return -1;
	}
	printf("Valid C2 and T2 passed\n");
	
	// Attempt to cancel


	
	return 0;
}
