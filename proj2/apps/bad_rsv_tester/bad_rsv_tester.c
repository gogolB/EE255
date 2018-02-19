#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <time.h>

#define MS_TO_NS 1000*1000

void printTestHeader(const char* msg)
{
	printf("--------------------------------\n");
	printf("\t%s\n",msg);
	printf("--------------------------------\n");
}




int main(void)
{
	//struct timespec t1, t2;
	struct timespec C, T;
	

	int r;
	// Test Set C to zero;
	C.tv_sec = 0;
	C.tv_nsec = 0;

	// Test Set T to zero;
	T.tv_sec = 0;
	T.tv_nsec = 0;

	// Canceling a call that doesn't exist.
	printTestHeader("Canceling a call that doesn't exist");
	r = syscall(398,0);
	if(r != -1)
	{
		printf("Test didn't fail. Something went wrong\n");
		return -1;
	}
	printf("Canceling a non existing sys call worked!\n");

	
	// Null C
	printTestHeader("C = NULL");
	r = syscall(397,0,NULL,&T);
	if(r != -1)
	{
		printf("Test didn't fail, Something went wrong\n");
		return -1;
	}
	printf("C Null test passed\n");

	// NULL T
	printTestHeader("T = NULL");
	r = syscall(397,0,&C,NULL);
	if(r != -1)
	{
		printf("Test didn't fail, something went wrong, r=%d\n",r);
		return -1;
	}
	printf("T Null test passed\n");

	// Bad C param
	printTestHeader("C = 0");
	r = syscall(397, 0, &C, &T);
	if(r != -1)
	{
		printf("Test didn't fail.\n");
		return -1;
	}
	printf("Bad C param passed\n");

	C.tv_sec = 1;
	
	// Bad T param test
	printTestHeader("T = 0");
	r = syscall(397, 0, &C, &T);
	if(r != -1)
	{
		printf("Test Didn't fail\n");
		return -1;
	}
	printf("Bad T param passed\n");

	// Bad T param test
	printTestHeader("Wait for next period");
	r = syscall(399);
	if(r != -1)
	{
		printf("Test Didn't fail\n");
		return -1;
	}
	printf("Wait for test reservation passed\n");
	
	return 0;
}
