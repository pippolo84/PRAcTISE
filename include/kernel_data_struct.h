/*
 * Copyright © 2012  Fabio Falzoi, Juri Lelli, Giuseppe Lipari
 *
 * This file is part of PRAcTISE.
 *
 * PRAcTISE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * PRAcTISE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.
 */

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
