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

#ifndef __COMMON_OPS_H
#define __COMMON_OPS_H

#include <linux/types.h>
#include <stdio.h>
#include <pthread.h>

#include "parameters.h"
#include "rq_heap.h"
#include "kernel_data_struct.h"

#include "cpumask.h"
#include "cpupri.h"

struct data_struct_ops {
	void (*data_init) (void *s, int nproc, int (*cmp_dl)(__u64 a, __u64 b));
	void (*data_cleanup) (void *s);

	/*
	 * Update CPU state inside the data structure
	 * after a preemption
	 */
	int (*data_preempt) (void *s, int cpu, __u64 dline, int is_valid);
	/*
	 * Update CPU state inside the data structure
	 * after a task finished 
	 */
	int (*data_finish) (void *s, int cpu, __u64 dline, int is_valid);
	/*
	 * data_find should find the best CPU where to push
	 * a task and/or find the best task to pull from
	 * another CPU
	 */
	int (*data_find) (void *s);
	int (*data_max) (void *s);

	void (*data_load) (void *s, FILE *f);
	void (*data_save) (void *s, int nproc, FILE *f);
	void (*data_print) (void *s, int nproc);

	int (*data_check) (void *s, int nproc);
	int (*data_check_cpu) (void *s, int cpu, __u64 dline);
};

extern struct data_struct_ops *dso;
extern void *push_data_struct, *pull_data_struct;
extern struct rq *cpu_to_rq[];

int __dl_time_before(__u64 a, __u64 b);

int __dl_time_after(__u64 a, __u64 b);

int __prio_higher(int a, int b);

int __prio_lower(int a, int b);

#ifdef SCHED_DEADLINE
void task_init(struct task_struct *t, __u64 dline, int pid);
#endif

#ifdef SCHED_RT
void task_init(struct task_struct *t, int prio, int runtime, int pid);
#endif

void rq_init (struct rq *rq, int cpu, struct root_domain *rd, FILE *f);

void rq_destroy (struct rq *rq);

void rq_lock (struct rq *rq);

void rq_unlock (struct rq *rq);

struct rq_heap_node* rq_peek (struct rq *rq);

struct rq_heap_node* rq_take (struct rq *rq);

struct task_struct* rq_node_task_struct(struct rq_heap_node* h);

void add_task_rq(struct rq* rq, struct task_struct* task);

int rq_pull_tasks(struct rq* this_rq);

int rq_push_tasks(struct rq* this_rq);

int rq_check(struct rq *rq);

void rq_print(struct rq *this_rq, FILE *out);

#endif /*__COMMON_OPS_H */
