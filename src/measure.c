#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "measure.h"
#include "parameters.h"

#ifdef MEASURE_CYCLE
	MEASURE_VARIABLE(cycle)
	ALL_COUNTER(cycle)
#endif

#ifdef MEASURE_SLEEP
	MEASURE_VARIABLE(sleep)
	ALL_COUNTER(sleep)
#endif

#ifdef MEASURE_ENQUEUE_NUMBER
	ALL_COUNTER(enqueue_number)
#endif

#ifdef MEASURE_ENQUEUE_CYCLE
	MEASURE_VARIABLE(enqueue_cycle)
	ALL_COUNTER(enqueue_cycle)
#endif

#ifdef MEASURE_DEQUEUE_NUMBER
	ALL_COUNTER(dequeue_number)
#endif

#ifdef MEASURE_DEQUEUE_CYCLE
	MEASURE_VARIABLE(dequeue_cycle)
	ALL_COUNTER(dequeue_cycle)
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

#ifdef MEASURE_CPUPRI_FIND
	MEASURE_VARIABLE(cpupri_find)
	FAIL_COUNTER(cpupri_find)
	SUCCESS_COUNTER(cpupri_find)
	ALL_COUNTER(cpupri_find)
#endif

#ifdef MEASURE_CPUPRI_SET
	MEASURE_VARIABLE(cpupri_set)
	ALL_COUNTER(cpupri_set)
#endif

#define FNAME_LEN		100

/* tsc_cost global variables */
TICKS_TYPE tsc_cost[NR_CPUS];

/*
 * alloc_samples_array - alloc memory for
 * samples array
 * @samples_array:		pointers to array where
 * we will store the samples
 */
void alloc_samples_array(SAMPLES_TYPE *samples_array[NR_CPUS])
{
	int i;

	for(i = 0; i < NR_CPUS; i++){
		samples_array[i] = (SAMPLES_TYPE *)calloc(SAMPLES_MAX, sizeof(**samples_array));
		if(!samples_array[i]){
			fprintf(stderr, "calloc(): %s\n", strerror(errno));
			exit(1);
		}
	}
}

/*
 * free_samples_array - free memory for
 * samples array
 * @samples_array:		pointers to array that
 * we want to free
 */
void free_samples_array(SAMPLES_TYPE *samples_array[NR_CPUS])
{
	int i;

	for(i = 0; i < NR_CPUS; i++)
		free(samples_array[i]);
}

/*
 * set_tsc_cost - calculate how many CPU 
 * cycles are needed to read TSC and set
 * the corresponding tsc_cost static variable
 * @cpu:		index of CPU calculating cost
 */
void set_tsc_cost(const int cpu)
{
	uint64_t tsc_cost_start_ticks_high, tsc_cost_start_ticks_low;
	uint64_t tsc_cost_end_ticks_high, tsc_cost_end_ticks_low;
	TICKS_TYPE tsc_cost_start_ticks, tsc_cost_end_ticks;
	TICKS_TYPE elapsed, min_tsc_cost;
	int i;

	min_tsc_cost = ~((TICKS_TYPE)0);
	for(i = 0; i < CALIBRATION_CYCLES; i++){
		GET_START_TICKS(tsc_cost)
		GET_END_TICKS(tsc_cost)
		elapsed = tsc_cost_end_ticks - tsc_cost_start_ticks;
		if(elapsed < min_tsc_cost)
			min_tsc_cost = elapsed;
	}
	tsc_cost[cpu] = min_tsc_cost;
}

/*
 * get_tsc_cost - return the number of 
 * cycles needed to read TSC twice
 * @cpu:		index of CPU
 */
TICKS_TYPE get_tsc_cost(const int cpu)
{
	if(!tsc_cost[cpu])
		set_tsc_cost(cpu);

	return tsc_cost[cpu];
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
 * get_elapsed_ticks - calculate elapsed CPU clock 
 * cycles from start to end
 * @cpu:		index of CPU calculating elapsed ticks 
 * @start:	TSC value at the beginning of measurements
 * @end:		TSC value at the end of measurements
 */
TICKS_TYPE get_elapsed_ticks(const int cpu, const TICKS_TYPE start, const TICKS_TYPE end)
{
	TICKS_TYPE elapsed;

	/* TSC overflow */
	if(end < start){
		fprintf(stderr, "WARNING: end ticks value (%llu ticks) < start ticks value (%llu ticks) on CPU %d\n", end, start, cpu);
		return 0;
	}

	elapsed = end - start;
	if(elapsed < tsc_cost[cpu]){
		fprintf(stderr, "WARNING: elapsed time (%llu ticks) < tsc cost (%llu ticks) on CPU %d\n", elapsed, tsc_cost[cpu], cpu);
		return 0;
	}
	else
		return elapsed - tsc_cost[cpu];
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

/**
 * measure_stream_open - open a file where samples
 * will be stored
 * @name:		file name prefix
 * @online_cpus:	running cpus number
 */
FILE *measure_stream_open(char *name, const int online_cpus)
{
	char filename[FNAME_LEN];
	
	if(!name)
		return NULL;
	
	sprintf(filename, "out_%s_%d", name, online_cpus);
	return fopen(filename, "w");
}

/*
 * measure_print - print measure results helper function
 * @out:							output stream
 * @variable_name:		a string that identifies which variable we want to print
 * @cpu:							CPU id
 * @elapsed:					array of samples (elapsed measure time)
 * @n_all:						number of samples
 */
void measure_print(FILE *out, char *variable_name, int cpu, SAMPLES_TYPE *elapsed[NR_CPUS], SAMPLES_TYPE *n_all)
{
	long long unsigned i;

	//fprintf(out, "[%d]: %s results\n", cpu, variable_name);
	//fprintf(out, "total number:\t%lu\n", n_all[cpu]);
	for(i = 0; i < n_all[cpu] && i < SAMPLES_MAX; i++)
		fprintf(out, "%7lu\n", elapsed[cpu][i]);
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

/*
 * account_print - print account measure (number of occurences of certain events) helper function
 * @out:							output stream
 * @variable_name:		a string that identifies which variable we want to print
 * @cpu:							CPU id
 * @n_all:						number of occurences
 */
void account_print(FILE *out, char *variable_name, int cpu, SAMPLES_TYPE *n_all)
{
	double cycle_period_secs = (double)CYCLE_LEN / 1000000;

	fprintf(out, "[%d]: %s occurences: %lu\n", cpu, variable_name, n_all[cpu]);
	fprintf(out, "[%d]: %s rate: %.0lf event/s\n", cpu, variable_name, n_all[cpu] / (NCYCLES * cycle_period_secs));
}
