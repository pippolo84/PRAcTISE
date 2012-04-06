#include <stdio.h>

#include "measure.h"
#include "cycle.h"
#include "parameters.h"

#ifdef MEASURE_CYCLE
	MEASURE_VARIABLE(cycle)
	ALL_COUNTER(cycle)
#endif

#ifdef MEASURE_SLEEP
	MEASURE_VARIABLE(sleep)
	ALL_COUNTER(sleep)
#endif

#ifdef MEASURE_PUSH_FIND
	MEASURE_VARIABLE(push_find)
	SUCCESS_COUNTER(push_find)
	FAIL_COUNTER(push_find)
	ALL_COUNTER(push_find)
#endif

#ifdef MEASURE_PULL_FIND
	MEASURE_VARIABLE(pull_find)
	SUCCESS_COUNTER(pull_find)
	FAIL_COUNTER(pull_find)
	ALL_COUNTER(pull_find)
#endif

#ifdef MEASURE_PUSH_PREEMPT
	MEASURE_VARIABLE(push_preempt)
	ALL_COUNTER(push_preempt)
#endif

#ifdef MEASURE_PULL_PREEMPT
	MEASURE_VARIABLE(pull_preempt)
	ALL_COUNTER(pull_preempt)
#endif

/* tsc_cost global variable */
TICKS_TYPE tsc_cost;

/*
 * set_tsc_cost - calculate how many CPU 
 * cycles are needed to read TSC and set
 * the corresponding tsc_cost static variable
 */
void set_tsc_cost()
{
	ticks t1, t2;
	TICKS_TYPE elapsed, min_tsc_cost;
	int i;

	min_tsc_cost = ~0ULL;
	for(i = 0; i < CALIBRATION_CYCLES; i++){
		t1 = get_ticks();
		t2 = get_ticks();
		elapsed = t2 - t1;
		if(elapsed < min_tsc_cost)
			min_tsc_cost = elapsed;
	}

	tsc_cost = min_tsc_cost;
}

/*
 * get_tsc_cost - return the number of 
 * cycles needed to read TSC twice
 */
TICKS_TYPE get_tsc_cost()
{
	if(!tsc_cost)
		set_tsc_cost();

	return tsc_cost;
}

/*
 * ticks_to_milliseconds - converts ticks in milliseconds
 * using the defined constant CPU_FREQ
 */
TICKS_TYPE ticks_to_milliseconds(const TICKS_TYPE ticks)
{
	return ticks / (CPU_FREQ / 1000ULL);
}

/*
 * ticks_to_microseconds - converts ticks in microseconds
 * using the defined constant CPU_FREQ
 */
TICKS_TYPE ticks_to_microseconds(const TICKS_TYPE ticks)
{
	return ticks / (CPU_FREQ / 1000000ULL);
}

/*
 * get_ticks - get the value of TSC
 */
TICKS_TYPE get_ticks(){
	return (TICKS_TYPE)getticks();
}

/*
 * get_elapsed_ticks - calculate elapsed CPU clock 
 * cycles from start to end
 * @start:	TSC value at the beginning of measurements
 * @end:		TSC value at the end of measurements
 */
TICKS_TYPE get_elapsed_ticks(const TICKS_TYPE start, const TICKS_TYPE end)
{
	TICKS_TYPE elapsed;

	elapsed = end - start;
	if(elapsed < tsc_cost){
		fprintf(stderr, "WARNING: elapsed time (%llu ticks) < tsc cost (%llu ticks)\n", elapsed, tsc_cost);
		return 0;
	}
	else
		return elapsed - tsc_cost;
}

/*
 * get_current_thread_time - get current
 * thread specific time
 * @t:		timespec variabile pointer where we
 * save current time
 */
void get_current_thread_time(struct timespec *t)
{
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, t);
}

/*
 * get_current_thread_time - get current
 * process specific time
 * @t:		timespec variabile pointer where we
 * save current time
 */
void get_current_process_time(struct timespec *t)
{
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, t);
}

/*
 * get_elapsed_time - calculate elapsed time
 * from start to end
 * @start:	measurement start time
 * @end:		measurement end time
 */
struct timespec get_elapsed_time(const struct timespec start, const struct timespec end)
{
	struct timespec temp;
	
	if (end.tv_nsec - start.tv_nsec < 0){
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = NANO_SECONDS_IN_SEC + end.tv_nsec - start.tv_nsec;
	}else{
		temp.tv_sec = end.tv_sec - start.tv_sec;
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}

	return temp;
}

/*
 * measure_print - print measure results helper function
 * @out:							output stream
 * @variable_name:		a string that identifies which variable we want to print
 * @cpu:							CPU id
 * @elapsed:					array of samples (elapsed measure time)
 * @n_all:						number of samples
 */
void measure_print(FILE *out, char *variable_name, int cpu, SAMPLES_TYPE elapsed[][NPROCESSORS], SAMPLES_TYPE *n_all)
{
	long long unsigned i;

	fprintf(out, "[%d]: %s results\n", cpu, variable_name);
	fprintf(out, "total number:\t%lu\n", n_all[cpu]);
	for(i = 0; i < n_all[cpu] && i < SAMPLES_MAX; i++)
		fprintf(out, "%7lu\n", elapsed[i][cpu]);
}

/*
 * outcome_print - print measure outcome (success/fail rate) helper function
 * @out:							output stream
 * @variable_name:		a string that identifies which variable we want to print
 * @cpu:							CPU id
 * @n_success:				number of succeded operations
 * @n_fail:						number of failed operations
 */
void outcome_print(FILE *out, char *variable_name, int cpu, SAMPLES_TYPE *n_success, SAMPLES_TYPE *n_fail)
{
	fprintf(out, "[%d]: %s outcome\n", cpu, variable_name);
	fprintf(out, "%s successful:\t%lu\n", variable_name, n_success[cpu]);
	fprintf(out, "%s failed:\t%lu\n", variable_name, n_fail[cpu]);
}
