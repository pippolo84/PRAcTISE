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

#include <string.h>

#include "common_ops.h"
#include "cpumask.h"
#include "cpupri.h"

#ifdef SCHED_RT

/*
 * priorities mapping from 0/140
 * scale to -1/102
 */
int convert_prio(int prio)
{
	int cpupri;

	if (prio == CPUPRI_INVALID)
		cpupri = CPUPRI_INVALID;
	else if (prio == MAX_PRIO)
		cpupri = CPUPRI_IDLE;
	else if (prio >= MAX_RT_PRIO)
		cpupri = CPUPRI_NORMAL;
	else
		cpupri = MAX_RT_PRIO - prio + 1;

	return cpupri;
}

/**
 * cpupri_find - find the best (lowest-pri) CPU in the system
 * @cp: The cpupri context
 * @p: The task
 * @lowest_mask: A mask to fill in with selected CPUs (or NULL)
 *
 * Note: This function returns the recommended CPUs as calculated during the
 * current invocation.  By the time the call returns, the CPUs may have in
 * fact changed priorities any number of times.  While not ideal, it is not
 * an issue of correctness since the normal rebalancer logic will correct
 * any discrepancies created by racing against the uncertainty of the current
 * priority configuration.
 *
 * Returns: (int)bool - CPUs were found
 */
int cpupri_find(struct cpupri *cp, struct task_struct *p,
		struct cpumask *lowest_mask)
{
	int                  idx      = 0;
	int                  task_pri = convert_prio(p->prio);

	if (task_pri >= MAX_RT_PRIO)
		return 0;

	for (idx = 0; idx < task_pri; idx++) {
		struct cpupri_vec *vec  = &cp->pri_to_cpu[idx];
		int skip = 0;

		if (!vec->count)
			skip = 1;
		/*
		 * When looking at the vector, we need to read the counter,
		 * do a memory barrier, then read the mask.
		 *
		 * Note: This is still all racey, but we can deal with it.
		 *  Ideally, we only want to look at masks that are set.
		 *
		 *  If a mask is not set, then the only thing wrong is that we
		 *  did a little more work than necessary.
		 *
		 *  If we read a zero count but the mask is set, because of the
		 *  memory barriers, that can only happen when the highest prio
		 *  task for a run queue has left the run queue, in which case,
		 *  it will be followed by a pull. If the task we are processing
		 *  fails to find a proper place to go, that pull request will
		 *  pull this task if the run queue is running at a lower
		 *  priority.
		 */
		__sync_synchronize();

		/* Need to do the mb for every iteration */
		if (skip)
			continue;

		if (cpumask_any_and(&p->cpus_allowed, vec->mask) >= nr_cpu_ids)
			continue;

		if (lowest_mask) {
			cpumask_and(lowest_mask, &p->cpus_allowed, vec->mask);

			/*
			 * We have to ensure that we have at least one bit
			 * still set in the array, since the map could have
			 * been concurrently emptied between the first and
			 * second reads of vec->mask.  If we hit this
			 * condition, simply act as though we never hit this
			 * priority level and continue on.
			 */
			if (cpumask_any(lowest_mask) >= nr_cpu_ids)
				continue;
		}

		return 1;
	}

	return 0;
}

/**
 * cpupri_set - update the cpu priority setting
 * @cp: The cpupri context
 * @cpu: The target cpu
 * @newpri: The priority (INVALID-RT99) to assign to this CPU
 *
 * Note: Assumes cpu_rq(cpu)->lock is locked
 *
 * Returns: (void)
 */
void cpupri_set(struct cpupri *cp, int cpu, int newpri)
{
	int                 *currpri = &cp->cpu_to_pri[cpu];
	int                  oldpri  = *currpri;
	int                  do_mb = 0;

	newpri = convert_prio(newpri);

	if (newpri == oldpri)
		return;

	/*
	 * If the cpu was currently mapped to a different value, we
	 * need to map it to the new value then remove the old value.
	 * Note, we must add the new value first, otherwise we risk the
	 * cpu being missed by the priority loop in cpupri_find.
	 */
	if (newpri != CPUPRI_INVALID) {
		struct cpupri_vec *vec = &cp->pri_to_cpu[newpri];

		cpumask_set_cpu(cpu, vec->mask);
		/*
		 * When adding a new vector, we update the mask first,
		 * do a write memory barrier, and then update the count, to
		 * make sure the vector is visible when count is set.
		 */
		__sync_fetch_and_add(&vec->count, 1);
		do_mb = 1;
	}
	if (oldpri != CPUPRI_INVALID) {
		struct cpupri_vec *vec  = &cp->pri_to_cpu[oldpri];

		/*
		 * Because the order of modification of the vec->count
		 * is important, we must make sure that the update
		 * of the new prio is seen before we decrement the
		 * old prio. This makes sure that the loop sees
		 * one or the other when we raise the priority of
		 * the run queue. We don't care about when we lower the
		 * priority, as that will trigger an rt pull anyway.
		 *
		 * We only need to do a memory barrier if we updated
		 * the new priority vec.
		 */
		if (do_mb)
			__sync_synchronize();

		/*
		 * When removing from the vector, we decrement the counter first
		 * do a memory barrier and then clear the mask.
		 */
		__sync_fetch_and_sub(&vec->count, 1);
		cpumask_clear_cpu(cpu, vec->mask);
	}

	*currpri = newpri;
}

/**
 * cpupri_init - initialize the cpupri structure
 * @cp: The cpupri context
 *
 * Returns: -1 if memory fails, 0 otherwise
 */
int cpupri_init(struct cpupri *cp)
{
	int i;

	memset(cp, 0, sizeof(*cp));

	for (i = 0; i < CPUPRI_NR_PRIORITIES; i++) {
		struct cpupri_vec *vec = &cp->pri_to_cpu[i];

		vec->count = 0;
		if (!alloc_cpumask_var(&vec->mask))
			goto cleanup;
	}

	struct cpumask cpu_possible_bits;
	cpumask_setall(&cpu_possible_bits);
	for_each_cpu(i, &cpu_possible_bits)
		cp->cpu_to_pri[i] = CPUPRI_INVALID;
	return 0;

cleanup:
	for (i--; i >= 0; i--)
		free_cpumask_var(cp->pri_to_cpu[i].mask);
	return -1;
}

/**
 * cpupri_cleanup - clean up the cpupri structure
 * @cp: The cpupri context
 */
void cpupri_cleanup(struct cpupri *cp)
{
	int i;

	for (i = 0; i < CPUPRI_NR_PRIORITIES; i++)
		free_cpumask_var(cp->pri_to_cpu[i].mask);
}

#endif /* SCHED_RT */
