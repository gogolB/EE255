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

EXPORT_SYMBOL(rsv_getPolicy);
EXPORT_SYMBOL(rsv_setPolicy);

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

void printCPUTaskList(int cpuid)
{
	struct task_time *tt;
	long long int util, totalUtil;
	printk(KERN_INFO"[RSV-DEBUG] Printing All Tasks on CPU %d\n[RSV-DEBUG] PID\t(C,T)\tUtil\n", cpuid);
	if(CPU_Head[cpuid] == NULL)
	{
		printk(KERN_INFO"[RSV-DEBUG] No Tasks On this CPU\n");
	}
	else
	{
		totalUtil = 0;
		tt = CPU_Head[cpuid];
		while(tt != NULL)
		{
			util = getUtil(&tt->C, &tt->T);
			printk(KERN_INFO"[RSV-DEBUG] %u\t(%ld, %ld)\t%lld\n", tt->pid, tt->C.tv_nsec, tt->T.tv_nsec, util);
			
			totalUtil += util;
			
			tt = tt->next;
		}
		printk(KERN_INFO"[RSV-DEBUG] CPU %d CURRENT UTIL: %lld", cpuid, totalUtil);
	}
}

// ******************************************************************************************************************************************************************
/*
static int RTT(int cpuid, struct timespec *C, struct timespec *T)
{
	int util = 0; 
	int totalUtil = 0;
	int numOfHPTasks = 0;
	int i = 0;
	long long int R, R_next, sumOfHPTasks;
	
	struct  task_time *tt;
	

	// Get the util
	util = getUtil(C,T);
	printk(KERN_INFO"[RTT] Running RTT on CPU %d with (%ld, %ld) and util %d\n", cpuid, C->tv_nsec, T->tv_nsec, util);


	// First check total util
	totalUtil = 0;	
	tt = CPU_Head[cpuid];
	while(tt != NULL)
	{
		if(C_lessthan_T(&tt->T, T))
			numOfHPTasks++;
		
		totalUtil += tt->util;
		tt = tt->next;
	}
	
	// Utilization is above 100% can't schedule.
	if((totalUtil + util) > 100)
	{
		printk(KERN_WARNING"[RTT] Utilization on this CPU would be over 100, can't schedule\n");
		return 0;
	}
	// Utilization is below 100% which means we need to run the RTT
	printk(KERN_INFO"[RTT] Current util on CPU %d is %d", cpuid, totalUtil);

	// R0 = C;
	R = C->tv_nsec;
	tt = CPU_Head[cpuid];

	printk(KERN_INFO"[RTT] Running RTT on CPU %d with numHPTasks %d\n", cpuid, numOfHPTasks);
 	while(R <= T->tv_nsec)
	{
		tt = CPU_Head[cpuid];
		sumOfHPTasks = 0;
		for(i = 0; i < numOfHPTasks; i++)
		{
			sumOfHPTasks += div_with_ceil(R,tt->T.tv_nsec)*tt->C.tv_nsec;
			tt = tt->next;
		}

		// R_(K+1) = C + Sum Of HP Tasks.
		R_next = C->tv_nsec + sumOfHPTasks;

		printk(KERN_INFO"[RTT] R(k+1)[%lld] = C[%ld] + sumHP[%lld]\n",R_next, C->tv_nsec, sumOfHPTasks);

		// Break condition.
		if (R_next == R)
			break;
		else
			R = R_next;
	}
	
	if(R <= T->tv_nsec)
	{
		// Test passed.
		return 1;
	}

	return 0;
}*/

static int RTT_PID(int cpuid, struct timespec *C, struct timespec *T, pid_t pid)
{
	int util = 0; 
	int totalUtil = 0;
	int numOfHPTasks = 0;
	int i = 0;
	long long int R, R_next, sumOfHPTasks;
	
	struct  task_time *tt;
	

	// Get the util
	util = getUtil(C,T);
	printk(KERN_INFO"[RTT-PID]Running RTT_PID on CPU %d with (%ld, %ld) and util %d\n", cpuid, C->tv_nsec, T->tv_nsec, util);


	// First check total util
	totalUtil = 0;	
	tt = CPU_Head[cpuid];
	while(tt != NULL)
	{
		if(C_lessthan_T(&tt->T, T))
			numOfHPTasks++;
		
		// If it us us, ignore it.
		if(tt->pid != pid)
			totalUtil += tt->util;
			
		tt = tt->next;
	}
	
	// Utilization is above 100% can't schedule.
	if((totalUtil + util) > 100)
	{
		printk(KERN_WARNING"[RTT-PID]Utilization on this CPU would be over 100, can't schedule\n");
		return 0;
	}
	// Utilization is below 100% which means we need to run the RTT
	printk(KERN_INFO"[RTT-PID]Current util on CPU %d is %d", cpuid, totalUtil);

	// R0 = C;
	R = C->tv_nsec;
	tt = CPU_Head[cpuid];

	printk(KERN_INFO"[RTT-PID]Running RTT on CPU %d with %d Higher Priority Tasks\n", cpuid, numOfHPTasks);
 	while(R <= T->tv_nsec)
	{
		tt = CPU_Head[cpuid];
		
		sumOfHPTasks = 0;
		for(i = 0; i < numOfHPTasks; i++)
		{
			// Should never happen, but if it is us, ignore it.
			if(tt->pid != pid)
				sumOfHPTasks += div_with_ceil(R,tt->T.tv_nsec)*tt->C.tv_nsec;
			
			tt = tt->next;
		}

		// R_(K+1) = C + Sum Of HP Tasks.
		R_next = C->tv_nsec + sumOfHPTasks;

		printk(KERN_INFO"[RTT-PID] R(k+1)[%lld] = C[%ld] + sumHP[%lld]\n",R_next, C->tv_nsec, sumOfHPTasks);

		// Break condition.
		if (R_next == R)
			break;
		else
			R = R_next;
	}
	
	if(R <= T->tv_nsec)
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
	int i = 0;
	// Added for debugging purposes
	for(i = 0; i < 4; i++)
		printCPUTaskList(i);
	
	if(CPU_Head[cpuid] == NULL)
	{
		// First task we can run it by default.
		printk(KERN_INFO"No Tasks on CPU: %d, can add by default.\n", cpuid);
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
		printk(KERN_INFO"Tasks on CPU: %d, Need to run RTT.\n", cpuid);
		if(1)
		{	
			printk(KERN_INFO"[RSV] PID %u adding to reservation for testing.\n", pid);
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
				printk("[RSV] Adding Task to head node.\n");
				tt->next = CPU_Head[cpuid];
				CPU_Head[cpuid]->prev = tt;
				CPU_Head[cpuid] = tt;
				goto totalRTTTest;
			}

			// Need to search.
			tts = CPU_Head[cpuid];
			while(tts != NULL)
			{
				if(C_lessthan_T(T, &tts->T))
				{
					printk("[RSV] Adding Task to middle of list.\n");
					tt->next = tts;
					tts->prev = tt;
					tts = tt;
					goto totalRTTTest;
				}
				else
				{
					if(tts->next == NULL)
						break;
					else
						tts = tts->next;
				}
			}
			printk("[RSV] Adding Task to end node.\n");
			// We got to the end, add it to the end.
			tts->next = tt;
			tt->prev = tts;
			// We can skip the total test because it was added to the end so implicitly all the other ones must be okay.
			goto validResult;

			totalRTTTest:
			// Make sure everything else is still schedualible.
			printk(KERN_INFO"[RSV] PID %u and has been added to reservation on CPU: %d. Checking all task.\n", pid, cpuid);
			tts = CPU_Head[cpuid];
			while(tts != NULL)
			{
				if(RTT_PID(cpuid, &tts->C, &tts->T, tts->pid) == 1)
				{
					tts = tts->next;
				}
				else
				{
					printk(KERN_INFO"[RSV] PID %u failed checking RTT of all other tasks on CPU: %d.\n", pid, cpuid);
					// One of the earlier tasks failed the RTT.
					sys_cancel_rsv(pid);
					return 0;

				}
			
			}
			
			validResult:
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
	printk(KERN_INFO"[RFLL] Removing from linked list\n");
	tt = CPU_Head[cpuid];
	while(tt != NULL)
	{
		if(tt->pid == pid)
		{
			printk(KERN_INFO"[RFLL] Found the PID\n");
			// We found what we were looking for.
			// Set the previous one's next to this one.
			if(tt->prev != NULL) // Deals with head node issue.
				tt->prev->next = tt->next;
			else 
				CPU_Head[cpuid] = CPU_Head[cpuid]->next;

			if(tt->next != NULL)
				tt->next->prev = tt->prev;
			
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
	int i;
	struct task_time *tt;
	// Array to contain Utils of all CPUs and associated core number. Makes sorting easier.
	int CPUutils[4][2];


	printk(KERN_INFO"[RSV] Attempting to find CPU for pid %u\n",pid);

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
		
		printk(KERN_INFO"[RSV] Util of CPU %d is %d\n", cpuid, totalUtil);
		
	}
	// This means we have at least one core
	// Sort utils in ascending order(stack exchange solution) := CPUutils
	// Now, start from the end of this array (position 3) and decrement until (3 - numPossibleCores)
	if(task_partitioning_heuristic == BEST_FIT)
	{ 
		printk(KERN_INFO"[RSV] Using BEST FIT\n");
		// Sort The CPU's in accending order of utilization
		sortUtils(CPUutils,4);
		// Go through and pick the CPU with the least free space.
		for(i = 3; i >= 0; i--)
		{
			printk(KERN_INFO"[RSV] Checking CPU %d with util %d\n", CPUutils[i][1], CPUutils[i][0]);
			if(CPUutils[i][0] + util <= 100)
			{
				cpuid = CPUutils[i][1];
				if(canRunOnCPU(pid,cpuid,C,T))
					return cpuid;
			}
			else
			{
				printk(KERN_INFO"[RSV] Task didn't fit.\n");
			}
		}
	}
	else if(task_partitioning_heuristic == WORST_FIT)
	{
		printk(KERN_INFO"[RSV] Using WORST FIT\n");
		// Sort the CPU's in accending order of utilization.
		sortUtils(CPUutils,4);
		// Go through and pick the CPU with the most free space.
		for(i = 0; i < 4; i++)
		{
			printk(KERN_INFO"[RSV] Checking CPU %d with util %d\n", CPUutils[i][1], CPUutils[i][0]);
			if(CPUutils[i][0] + util <= 100)
			{
				cpuid = CPUutils[i][1];
				if(canRunOnCPU(pid,cpuid,C,T))
					return cpuid;
			}
			else
			{
				printk(KERN_INFO"[RSV] Task didn't fit.\n");
			}
		}
		
	}
	else if(task_partitioning_heuristic == FIRST_FIT)
	{
		printk(KERN_INFO"[RSV] Using FIRST FIT\n");
		// Go through all the CPU's and pick the first one that the task will fit on.
		for(i = 0; i < 4; i++)
		{
			printk(KERN_INFO"[RSV] Checking CPU %d with util %d\n", CPUutils[i][1], CPUutils[i][0]);
			if(CPUutils[i][0] + util <= 100)
			{
				cpuid = CPUutils[i][1];
				if(canRunOnCPU(pid,cpuid,C,T))
					return cpuid;
			}
			else
			{
				printk(KERN_INFO"[RSV] Task didn't fit.\n");
			}
		}
	}
	else
	{
		printk(KERN_ALERT"[RSV] UNKNOWN BINNING POLICY %d\n",task_partitioning_heuristic);
	}
	// We didn't find an available CPU
	printk(KERN_INFO"[RSV] Failed to find CPU for pid %u\n",pid);
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
			printk(KERN_ALERT"[RSV] Failed to schedule task.\n");
			return -1;
		}
	}
	else
	{
		printk(KERN_ALERT"[RSV] Attempting to allocate PID %u to CPU %d\n", target_pid, cpuid);
		// Allows for removal later.
		task->rsv_task = 1;
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
	printk(KERN_INFO"[RSV] Reservation Set. CPU affinity Set. \n");

	return 0;
}

asmlinkage int sys_cancel_rsv(pid_t pid)
{
	pid_t target_pid;
	struct task_struct *task;
	struct pid *pid_struct;
	int i = 0;

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
		
		// For Debugging
		for(i = 0; i < 4; i++)
			printCPUTaskList(i);
		return 0;
	}
	else
	{
		// Task was not reserved, no need to do anything.
		printk(KERN_WARNING"[RSV] No Reservation for PID: %u\n",target_pid);
		return -1;
	}
}

