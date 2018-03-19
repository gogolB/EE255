#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

static volatile int keepRunning = 1;
void intHandler(int dummy)
{
	keepRunning = 0;
}

int main(void)
{
	
	// Register sig int handler.
	signal(SIGINT, intHandler);	
	printf("Created Test Process with PID %u\n", getpid());
	while (keepRunning) {
		sleep(1);
	}

	return 0;
}
