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
 * along with PRAcTISE.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PARAMETERS_H
#define __PARAMETERS_H

/* verbose mode */
//#define VERBOSE

/* debug mode */
//#define DEBUG

/*
 * choose from:
 * a) deadline based scheduling
 * b) priority based scheduling
 */
#define SCHED_DEADLINE
//#define SCHED_RT

/* default to SCHED_DEADLINE */
#if !defined(SCHED_DEADLINE) && !defined(SCHED_RT)
	#define SCHED_DEADLINE
#endif

#if defined(SCHED_DEADLINE) && defined(SCHED_RT)
	#error "SCHED_DEADLINE and SCHED_RT macro defined together"
#endif

/* 
 * if checker find an error and 
 * this macro id defined, 
 * simulation will stop immediately 
 */
//#define EXIT_ON_ERRORS

/* 
 * Activate all measurements. 
 * Beware that some measurements will 
 * slow others if they're nested. 
 * So it's not recommended to activate all
 * in the same simulation
 */
//#define MEASURE_ALL

/*
 * measure how much time CPUs 
 * spend sleeping
 */ 
//#define MEASURE_SLEEP

/*
 * measure how much time a
 * simulation takes (it must
 * be extremely close to 10ms
 * if the simulation runs
 * correctly)
 */
//#define MEASURE_CYCLE

/*
 * measure number of occurences
 * of enqueues on a certain runqueue
 */
#define MEASURE_ENQUEUE_NUMBER

/*
 * measure how much time an
 * enqueue on a runqueue takes
 */
#define MEASURE_ENQUEUE_CYCLE

/*
 * measure number of occurences
 * of enqueues on a certain runqueue
 */
#define MEASURE_DEQUEUE_NUMBER

/*
 * measure how much time a
 * dequeue on a runqueue takes
 */
#define MEASURE_DEQUEUE_CYCLE

/*
 * measure how much time
 * a find operation takes
 * on the push structure
 */
#define MEASURE_PUSH_FIND

/*
 * measure how much time
 * a find operation takes
 * on the push structure
 */
#define MEASURE_PULL_FIND

/*
 * measure how much time
 * a preempt operation takes
 * on the push structure
 */
#define MEASURE_PUSH_PREEMPT

/*
 * measure how much time
 * a find operation takes
 * on the pull structure
 */
#define MEASURE_PULL_PREEMPT

/*
 * measure how much time
 * a cpupri_set operation
 * takes
 */
//#define MEASURE_CPUPRI_SET

/*
 * measure how much time
 * a cpupri_find operation
 * takes
 */
//#define MEASURE_CPUPRI_FIND

/* CPUs number */
#define NR_CPUS					48
/* simulation cycles number */
#define NCYCLES					1000
/* simulation cycle period [us] */
#define CYCLE_LEN				10000	/* 1 cycle = 10ms simulated time */
#define DMIN						10
#define DMAX						100
#define RUNTIMEMIN			5
#define RUNTIMEMAX			15

#define LOGNAME_LEN		16

#define MAX_DL	~0ULL

#endif
