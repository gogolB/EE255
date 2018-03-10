#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char * argv[])
{
	int pid;
	int C_ns;
	int T_ns;
	int cpuid;
	int returnValue;
	struct timespec C,T;
	
	// First check the number of arguements, make sure it matches what was expected.
	if(argc != 5)
	{
		printf("Invalid number of params! Example of use: ./reserve 1101 250 500 1\n\t./reserve [pid] [C] [T] [CPUID [0,3]]\n");
		return -1;
	}

	
	// Get PID.
	pid = atoi(argv[1]);
	// Get C
	C_ns = atoi(argv[2]);
	// Get T
	T_ns = atoi(argv[3]);
	// Get CPUID
	cpuid = atoi(argv[4]);

	// Create the C and T timestructs.
	C.tv_sec = 0;
	C.tv_nsec = C_ns;
	T.tv_sec = 0;
	T.tv_nsec = T_ns;

	// Call the system call and check the result
	returnValue = syscall(397, pid, &C, &T, cpuid);

	if(returnValue != 0)
	{
		printf("Somekind of error happened when setting RSV with params (PID: %d, C: %d, T: %d, CPUID: %d)\nCheck System log (dmesg | tail)\n", pid, C_ns, T_ns, cpuid);
		return -1;
	}
	
	return 0;

}
