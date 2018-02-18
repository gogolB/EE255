#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <time.h>

#define MS_TO_NS 1000*1000

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
	r = syscall(398,0);
	if(r != -1)
	{
		printf("Test didn't fail. Something went wrong\n");
	}
	printf("Canceling a non existing sys call worked!\n");

	// Bad C param
	r = syscall(397, 0, &C, &T);
	if(r != -1)
	{
		printf("Test didn't fail.\n");
	}
	printf("Bad C param passed\n");

	C.tv_sec = 1;
	
	// Bad T param test
	r = syscall(397, 0, &C, &T);
	if(r != -1)
	{
		printf("Test Didn't fail\n");
	}
	printf("Bad T param passed\n");
}
