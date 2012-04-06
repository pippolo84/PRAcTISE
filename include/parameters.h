#ifndef __PARAMETERS_H
#define __PARAMETERS_H

/* verbose mode */
//#define VERBOSE

/* debug mode */
//#define DEBUG

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

#define NPROCESSORS    48
#define NCYCLES        10 /* 1 cycle = 10ms simulated time */
#define DMIN           10
#define DMAX           100
#define WAITCYCLE      10000

#define LOGNAME_LEN		16

#define MAX_DL	~0ULL

#endif
