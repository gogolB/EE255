#ifndef __rsv_header
#define __rsv_header
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/time.h>

#define BEST_FIT 0
#define WORST_FIT 1
#define FIRST_FIT 2

// CPU COUNT
#define CPU_C 4

struct task_time{
	
	struct timespec C;
	struct timespec T;

	int util;

	pid_t pid;
	
	struct task_time * prev;
	struct task_time * next;
};

//static int task_partitioning_heuristic = BEST_FIT;

long long int div_with_ceil(long long int x, long long int y);

int canRunOnCPU(pid_t pid, int cpuid, struct timespec *C, struct timespec *T);

int sys_cancel_rsv(pid_t pid);
int sys_set_rsv(pid_t pid, struct timespec *C, struct timespec *T, int cpuid);

#endif
