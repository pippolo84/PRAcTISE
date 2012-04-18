#ifndef __KERNEL_DATA_STRUCT_H
#define __KERNEL_DATA_STRUCT_H

#include <linux/types.h>
#include <pthread.h>

#include "cpumask.h"
#include "cpupri.h"
#include "rq_heap.h"

struct task_struct {
	int pid;
#ifdef SCHED_DEADLINE
	__u64 deadline;
#endif
#ifdef SCHED_RT
	int prio;
	cpumask_t cpus_allowed;
	int runtime;
#endif
	struct rq *rq;
};

struct rq {
	int cpu;
	struct rq_heap heap;
	pthread_spinlock_t lock;
	/* cache values */
#ifdef SCHED_DEADLINE
	__u64 earliest, next;
#endif
#ifdef SCHED_RT
	int highest, next;
	struct cpupri *cpupri;
#endif
	int nrunning, overloaded;
	struct root_domain *rd;
	FILE *log;
};

struct root_domain {
#ifdef SCHED_RT
	int rto_count;		/* operations on this MUST be ATOMIC */
	/*
	 * The "RT overload" flag: it gets set if a CPU has more than
	 * one runnable RT task.
	 */
	cpumask_var_t rto_mask;
	struct cpupri cpupri;
#endif
};

#endif
