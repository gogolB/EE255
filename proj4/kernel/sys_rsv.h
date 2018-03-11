#ifndef __rsv_header
#define __rsv_header
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/time.h>
// ******************************************************************************************************************************************************************
// Task part Policies. 
#define BEST_FIT 0
#define WORST_FIT 1
#define FIRST_FIT 2
// ******************************************************************************************************************************************************************
// CPU COUNT
#define CPU_C 4
// ******************************************************************************************************************************************************************
struct task_time{
	
	struct timespec C;
	struct timespec T;

	int util;

	pid_t pid;
	
	struct task_time * prev;
	struct task_time * next;
};

// ******************************************************************************************************************************************************************
/*
 * Performs the operation ceil(x/y). Used in RTT.
 */
long long int div_with_ceil(long long int x, long long int y);

/*
 * Checks to see if a specific task can be run on given cpu. Return 1 if successful, and 0 otherwise.
 */
int canRunOnCPU(pid_t pid, int cpuid, struct timespec *C, struct timespec *T);
// ******************************************************************************************************************************************************************
/*
 * Attempts to cancel a reservation. Return 0 on success, -1 otherwise.
 */
int sys_cancel_rsv(pid_t pid);

/*
 * Attemps to set a reservation. Returns 0 on sucess, -1 otherwise.
 */
int sys_set_rsv(pid_t pid, struct timespec *C, struct timespec *T, int cpuid);
// ******************************************************************************************************************************************************************
/*
 * Tries to find a CPU using the task part policy set. Return -1 on failure.
 */
int findCPU(pid_t pid, struct timespec *C, struct timespec *T);
// ******************************************************************************************************************************************************************
/*
 * Gets the current Policy.
 */
int rsv_getPolicy(void);

/*
 * Sets a new policy. Return 0 if successful, -1 otherwise.
 */
int rsv_setPolicy(int newPolicy);

#endif
