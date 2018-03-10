#ifndef __rsv_header
#define __rsv_header
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/time.h>
#include <linux/math.h>

int sys_cancel_rsv(pid_t pid);
int sys_set_rsv(pid_t pid, struct timespec *C, struct timespec *T, int cpuid);

#endif
