#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/init.h>
#include <linux/cpumask.h>
#include <linux/slab.h>

#include "sys_rsv.h"

// included for sorting
//#include <algorithm>

#define UINT64 long long int 
#define UINT32 long int 

// ******************************************************************************************************************************************************************
// Please note that there should be one CPU_Head per cpu.
// This linklist is sorted in accending order of T.
struct task_time* CPU_Head[CPU_C];

static char setup = 0;

static int task_partitioning_heuristic = BEST_FIT;
// ******************************************************************************************************************************************************************
int rsv_getPolicy(void)
{
	return task_partitioning_heuristic;
}

int rsv_setPolicy(int newPolicy)
{
	if(newPolicy != BEST_FIT && newPolicy != WORST_FIT && newPolicy != FIRST_FIT)
	{
		printk(KERN_WARNING"[RSV] Tried to set invalid policy. (%d)\n", newPolicy);
		return -1;
	}
	else
	{
		task_partitioning_heuristic = newPolicy;

		switch(newPolicy)
		{
			case BEST_FIT:
				printk(KERN_INFO"[RSV] Task Part Policy set to: BEST FIT\n");				
				break;

			case WORST_FIT:
				printk(KERN_INFO"[RSV] Task Part Policy set to: WORST FIT\n");				
				break;

			case FIRST_FIT:
				printk(KERN_INFO"[RSV] Task Part Policy set to: FIRST FIT\n");				
				break;
		}

		return 0;
	}
}
// ******************************************************************************************************************************************************************
static inline uint64_t lldiv(uint64_t dividend, uint32_t divisor)
{
    uint64_t __res = dividend;
    do_div(__res, divisor);
    return(__res);
}

static inline uint64_t getUtil(struct timespec *C, struct timespec *T)
{
	return lldiv(C->tv_nsec * 100, T->tv_nsec);
}

bool C_lessthan_T(struct timespec *C, struct timespec *T)
{
	if(C->tv_sec == T->tv_sec)
		return C->tv_nsec < T->tv_nsec;
	else
		return C->tv_sec < T->tv_sec; 
}

long long int div_with_ceil(long long int x, long long int y)
{
	if(x != 0)
		return 1 + lldiv(x - 1, y);
	else
		return lldiv(x + y - 1 , y);
}

// ******************************************************************************************************************************************************************

static inline int RTT(int cpuid, struct timespec *C, struct timespec *T)
{
	int util; 
	int totalUtil;
	int numOfHPTasks;
	int i;
	long long int R, R_next, sumOfHPTasks;
	
	struct  task_time *tt;
	

	// Get the util
	util = getUtil(C,T);

	// First check total util
	totalUtil = 0;	
	tt = CPU_Head[cpuid];
	while(tt != NULL)
	{
		if(C_lessthan_T(&tt->T, T))
			numOfHPTasks++;
		
		totalUtil += tt->util;
	}
	
	// Utilization is above 100% can't schedule.
	if((totalUtil + util) > 100)
		return 0;

	// Utilization is below 100% which means we need to run the RTT

	// R0 = C;
	R = C->tv_nsec;
	tt = CPU_Head[cpuid];

 	while(R < T->tv_nsec)
	{
		sumOfHPTasks = 0;
		for(i = 0; i < numOfHPTasks; i++)
		{
			sumOfHPTasks += div_with_ceil(R,tt->T.tv_nsec)*tt->C.tv_nsec;
			tt = tt->next;
		}

		// R_(K+1) = C + Sum Of HP Tasks.
		R_next = C->tv_nsec + sumOfHPTasks;

		// Break condition.
		if (R_next == R)
			break;
		else
			R = R_next;
	}
	
	if(R < T->tv_nsec)
	{
		// Test passed.
		return 1;
	}

	return 0;
}

int canRunOnCPU(pid_t pid, int cpuid, struct timespec *C, struct timespec *T)
{

	struct task_time* tt;
	struct task_time* tts;
	if(CPU_Head[cpuid] == NULL)
	{
		// First task we can run it by default.
		
		// Need to create the struct and update everything else.
		tt = (struct task_time*)kmalloc(sizeof(struct task_time), GFP_KERNEL);

		// Add in the PID (For removal purposes)
		tt->pid = pid;
		
		// Add in all the C and T data.				
		tt->C.tv_sec = C->tv_sec;
		tt->C.tv_nsec = C->tv_nsec;
		tt->T.tv_sec = T->tv_sec;
		tt->T.tv_nsec = T->tv_nsec;

		// Update the utilization
		tt->util = getUtil(C,T);

		// Update the linklist.
		tt->prev = NULL;
		tt->next = NULL;

		// Set as the head node.
		CPU_Head[cpuid] = tt;
		return 1;
	}
	else
	{
		// We now have to run a RTT test and make sure that it will fit. Only need to run on the new addition since the ones before were schedualible.
		if(RTT(cpuid, C, T) == 1)
		{	
			printk(KERN_INFO"[RSV] PID %u passed the RTT. Adding to reservation.\n", pid);
			// We passed the run time test.
			// Need to add to the Linklist in the right location. First create the object to add.
			tt = (struct task_time*)kmalloc(sizeof(struct task_time), GFP_KERNEL);

			// Add in the PID (For removal purposes)
			tt->pid = pid;
		
			// Add in all the C and T data.				
			tt->C.tv_sec = C->tv_sec;
			tt->C.tv_nsec = C->tv_nsec;
			tt->T.tv_sec = T->tv_sec;
			tt->T.tv_nsec = T->tv_nsec;

			// Update the utilization
			tt->util = getUtil(C,T);

			// Update the linklist.
			tt->prev = NULL;
			tt->next = NULL;

			// Check the head node, easy single case to swap.
			if(C_lessthan_T(T, &CPU_Head[cpuid]->T))
			{
				tt->next = CPU_Head[cpuid];
				CPU_Head[cpuid]->prev = tt;
				CPU_Head[cpuid] = tt;
				return 1;
			}

			// Need to search.
			// Skip CPU_Head because we did that above.
			tts = CPU_Head[cpuid]->next;
			while(tts != NULL)
			{
				if(C_lessthan_T(T, &tts->T))
				{
					tt->next = tts;
					tts->prev = tt;
					tts = tt;
					return 1;
				}
				else
				{
					if(tts->next == NULL)
						break;
					else
						tts = tts->next;
				}
			}

			// We got to the end, add it to the end.
			tts->next = tt;
			tt->prev = tts;

			printk(KERN_INFO"[RSV] PID %u and has been added to reservation on CPU: %d. Checking all task.\n", pid, cpuid);
			tts = CPU_Head[cpuid];
			while(tts != NULL)
			{
				if(RTT(cpuid, &tts->C, &tts->T) == 1)
					tts = tts->next;
				else
				{
					printk(KERN_INFO"[RSV] PID %u failed checking RTT of all other tasks on CPU: %d.\n", pid, cpuid);
					// One of the earlier tasks failed the RTT.
					sys_cancel_rsv(pid);
					return 0;

				}
			
			}
			
			return 1;
		}

		// Failed RTT.
		return 0;
	}

	
}

static inline void removeFromLinkList(int cpuid, pid_t pid)
{
	struct task_time* tt;
	// Get the head of the link list.
	tt = CPU_Head[cpuid];
	while(tt != NULL)
	{
		if(tt->pid == pid)
		{
			// We found what we were looking for.
			// Set the previous one's next to this one.
			if(tt->prev != NULL) // Deals with head node issue.
				tt->prev->next = tt->next;
			
			// Remove this one
			kfree(tt);
			break;
		}
		else
		{
			// Itterate along the list.
			tt = tt->next;
		}
	}
}

// Here's our insertion sort algorithm
// http://cforbeginners.com/insertionsort.html
void sortUtils(int unsorted[][2], int length)
{
	int i,j, temp1, temp2;
		
	for (i = 0; i < length; i++)
	{
		j = i;
		
		while (j > 0 && unsorted[j][0] < unsorted[j-1][0])
		{
			temp1 = unsorted[j][0];
			temp2 = unsorted[j][1];
			unsorted[j][0] = unsorted[j-1][0];
			unsorted[j][1] = unsorted[j-1][1];
			unsorted[j-1][0] = temp1;
			unsorted[j-1][1] = temp2;
			j--;
		}
	}
}
// ******************************************************************************************************************************************************************
// Stub Function. TODO: Finish this function.
int findCPU(pid_t pid, struct timespec *C, struct timespec *T)
{
	int util; 
	int totalUtil;

	int cpuid;

	int numPossibleCores = 0;
	int high = 0;
	int i;
	struct task_time *tt;
	// Array to contain Utils of all CPUs and associated core number. Makes sorting easier.
	int CPUutils[4][2];
	// Array containing sorted <util,cpu> pairs
	//int CPUsorted[4][2];
	// Calculate util needed for new task
	util = getUtil(C,T);
	// Calculate utils of all cores and store in array CPUutils
	for(cpuid = 0;cpuid <= 3; cpuid++)
	{
		totalUtil = 0;	
		tt = CPU_Head[cpuid];
		while(tt != NULL)
		{
			totalUtil += tt->util;
			tt = tt->next;
		}
		CPUutils[cpuid][0] = totalUtil;
		CPUutils[cpuid][1] = cpuid;
		// Check if CPU(cpuid) can handle new task
		if(totalUtil+util <= 100)
		{
			numPossibleCores++;
		}
		
	}
	// We now have an array of bools that show which cores (if any) can handle the new task
	// First, we shoud determine if any cores could schedule the task
	if(numPossibleCores <= 0)
		return -1;
	// This means we have at least one core
	// Sort utils in ascending order(stack exchange solution) := CPUutils
	//std::sortUtils(CPUutils,CPUutils + 4,comparator)
	sortUtils(CPUutils,4);   //******************************UNCOMMENT THSI LINE
	// Now, start from the end of this array (position 3) and decrement until (3 - numPossibleCores)
	high = 0 + numPossibleCores - 1;
	for(i = 0; i <= high; i++)
	{
		cpuid = CPUutils[i][1];
		if(canRunOnCPU(pid,cpuid,C,T))
			return cpuid;
	}
	// We didn't find an available CPU
	return -1;
}


// ******************************************************************************************************************************************************************
asmlinkage int sys_set_rsv(pid_t pid, struct timespec *C, struct timespec *T, int cpuid)
{
	pid_t target_pid;
	struct task_struct *task;
	struct pid *pid_struct;
	cpumask_t cpu_set;
	int returnValue;
	int targetCPUID;
	int i;

	if(!setup)
	{
		// Run setup stuff only on the first call.
		for(i = 0; i < CPU_C; i++)
		{
			CPU_Head[i] = NULL;
		}
		setup = 1;
	}

	// Check all the params for nulls
	if(C == NULL || T == NULL)
	{
		// We got bad addresses for C or T
		printk(KERN_ALERT"[RSV]Recieved bad address for C(0x%p) or T(0x%p)\n",C, T);
		return -1;
	}
	
	// Check and make sure that C isn't zero
	if(C->tv_sec == 0 && C->tv_nsec == 0)
	{
		printk(KERN_ALERT"[RSV]C is 0!\n");
		return -1;
	}

	// Check and make sure that T isn't zero
	if(T->tv_sec == 0 && T->tv_nsec == 0)
	{
		printk(KERN_ALERT"[RSV]T is 0!\n");
		return -1;
	}
		
	// Make sure that C < T or C/T is <= 1
	if(!C_lessthan_T(C,T))
	{
		printk(KERN_ALERT"[RSV]C is not <= T\n");
		return -1;
	}

	if(cpuid < -1 || cpuid > 3)
	{
		printk(KERN_ALERT"[RSV] CPUID %d is not a valid CPUID. Please pick a CPUID between 0 and 3\n", cpuid);
		return -1;
	}
	

	// Set the target PID.
	if(pid == 0)
	{
		printk(KERN_INFO"[RSV]Getting PID of calling task\n");
		target_pid = current->pid;
	}
	else
	{
		target_pid = pid;
	}

	printk(KERN_INFO"[RSV]Target PID: %u\n", target_pid);
	

	// Now we get the TCB
	pid_struct = find_get_pid(target_pid);
	if(pid_struct == NULL)
	{
		printk(KERN_ALERT"[RSV] failed to get a pid struct. Bad PID: %u\n",target_pid);
		return -1;
	}	
	
	task = (struct task_struct*)pid_task(pid_struct,PIDTYPE_PID);

	
	if(task->rsv_task == 1)
	{
		// Can not reserve a task that is already reserved.
		printk(KERN_ALERT"[RSV] This task is already reserved. PID: %u\n",target_pid);
		return -1;
	}


	if(cpuid == -1)
	{
                // Accept a new task iff partitioning heuristic is able to successfully allocate the 
		// new task to a CPU core. Allocation successful iff all tasks of target CPU core are 
		// guaranteed to be schedulable by rtt.
		// Use "Best-Fit" as partition heuristic. Find core with min remaining space among 
		// cores that can schedule task.
		targetCPUID = findCPU(pid,C,T);
		if(targetCPUID == -1)
		{
			printk(KERN_ALERT"[RSV] Feature unsupported, please pick a valid CPUID [0, 3]\n");
			return -1;
		}
	}
	else
	{
		if(canRunOnCPU(target_pid, cpuid, C, T) != 0)
		{
			targetCPUID = cpuid;
		}
		else
		{
			printk(KERN_ALERT"[RSV] Failed to allocate PID %u to CPU %d, failed RTT\n", target_pid, cpuid);
			return -1;
		}
	}	

	
	// After this point we can assume that the task is good and can be reserved.		
	
	printk(KERN_INFO"[RSV]Setting task to be reserved\n");
	// Set the flag for reservation
	task->rsv_task = 1;
	
	// Set all the C and T Values.
	task->C.tv_sec = C->tv_sec;
	task->C.tv_nsec = C->tv_nsec;
	task->T.tv_sec = T->tv_sec;
	task->T.tv_nsec = T->tv_nsec;

	// Set the CPU Affinity
	cpumask_clear(&cpu_set);
	cpumask_set_cpu(targetCPUID, &cpu_set);
	returnValue = sched_setaffinity(target_pid, &cpu_set);
	if(returnValue != 0)
	{
		printk(KERN_ALERT"[RSV] Failed to set Affinity. Error code: %d\n", returnValue);
		return -1;
	}
	task->cpuaffinity = targetCPUID;
	

	return 0;
}

asmlinkage int sys_cancel_rsv(pid_t pid)
{
	pid_t target_pid;
	struct task_struct *task;
	struct pid *pid_struct;

	if(pid == 0)
	{
		target_pid = current->pid;
	}
	else
	{
		target_pid = pid;
	}

	
	// Find the TCB
	pid_struct = find_get_pid(target_pid);
	if(pid_struct == NULL)
	{
		printk(KERN_ALERT"[RSV] Failed to find a pid struct. Bad PID:%u\n",target_pid);
		return -1;
	}
	
	task = (struct task_struct *)pid_task(pid_struct, PIDTYPE_PID);

	// Check if it is reserved.
	if(task->rsv_task == 1)
	{
		// Clear the flag.
		task->rsv_task = 0;
		printk(KERN_INFO"[RSV] Rsv canceled for PID: %u\n",target_pid);


		// Remove it from the link list.
		removeFromLinkList(task->cpuaffinity, pid);
		return 0;
	}
	else
	{
		// Task was not reserved, no need to do anything.
		printk(KERN_WARNING"[RSV] No Reservation for PID: %u\n",target_pid);
		return -1;
	}
}

