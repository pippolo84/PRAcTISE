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
//#define SCHED_DEADLINE
#define SCHED_RT

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
//#define MEASURE_ENQUEUE_NUMBER

/*
 * measure number of occurences
 * of enqueues on a certain runqueue
 */
//#define MEASURE_DEQUEUE_NUMBER

/*
 * measure how much time
 * a find operation takes
 * on the push structure
 */
//#define MEASURE_PUSH_FIND

/*
 * measure how much time
 * a find operation takes
 * on the push structure
 */
//#define MEASURE_PULL_FIND

/*
 * measure how much time
 * a preempt operation takes
 * on the push structure
 */
//#define MEASURE_PUSH_PREEMPT

/*
 * measure how much time
 * a find operation takes
 * on the pull structure
 */
//#define MEASURE_PULL_PREEMPT

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
