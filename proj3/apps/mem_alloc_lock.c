#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>


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


int main(int argc, char * argv[])
{

	int amt_mem;
	struct timespec t1, t2, diff;
	char *buff;
	int i;
	
	mlockall(MCL_CURRENT|MCL_FUTURE);
	
	if(argc != 2)
	{
		printf("Invalid number of arguments. Example call: ./mem_alloc 100000000\n");
		return -1;
	}


	amt_mem = atoi(argv[1]);
	
	buff = malloc(amt_mem);

	// Make sure the amount of memory we try to allocate is valid...
	if(buff == NULL)
	{
		printf("Failed to allocate memory.\n");
		return -1;
	}
	
	clock_gettime(CLOCK_REALTIME, &t1);
	for (i = 0; i < amt_mem; i += 4096)
		buff[i] = 1;

	clock_gettime(CLOCK_REALTIME, &t2);
	diff = timespec_diff(&t1, &t2);

	printf("Access time: %ldns\n",(diff.tv_sec*1000000000 + diff.tv_nsec));
	syscall(400,0);
	syscall(401,0);
	munlockall();
	free(buff);
}
