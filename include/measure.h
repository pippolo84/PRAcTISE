#ifndef __MEASURE_H
#define __MEASURE_H

#include <time.h>
#include <stdio.h>
#include <math.h>

#include "cycle.h"
#include "parameters.h"

/*
 * since we have to use a constant_tsc 
 * processor we need only to set here the 
 * maximum clock frequency available, no 
 * matter if cpu dynamic frequency scaling is
 * enabled (it only affects the benchmark
 * result, not the time measurement)
 */
#define CPU_FREQ							2400000000ULL

/*
 * As recommended by Intel we have to
 * repeat the rdtsc cost measurement 
 * at least 3 times.
 * See http://www.ccsl.carleton.ca/~jamuir/rdtscpm1.pdf
 * for more details
 */
#define CALIBRATION_CYCLES		3

/*
 * max number of samples recorded
 * for a specific measurement
 * variable
 */
#define SAMPLES_MAX						1000000

/* type used for samples storage */
#define SAMPLES_TYPE					long unsigned

/* 
 * type used for ticks storage. 
 * Since the TSC is implemented as
 * a 64-bit register in a large number
 * of architectures, this type
 * must be at least a 64 bit type
 */
#define TICKS_TYPE						long long unsigned

#define NANO_SECONDS_IN_SEC		1000000000

#ifdef MEASURE_ALL
	#define MEASURE_SLEEP
	#define MEASURE_CYCLE
	#define MEASURE_PUSH_FIND
	#define MEASURE_PULL_FIND
	#define MEASURE_PUSH_PREEMPT
	#define MEASURE_PULL_PREEMPT
#endif

#if defined(MEASURE_SLEEP) || defined(MEASURE_CYCLE) || \
	defined(MEASURE_PUSH_FIND) || defined(MEASURE_PULL_FIND) || \
	defined(MEASURE_PUSH_PREEMPT) || defined(MEASURE_PULL_PREEMPT)

	#define MEASURE
#endif

#define IDENTIFIER(prefix, name) prefix##name
#define TYPE_DECL(type, name) type name
#define ARRAY_DECL(name, number) name[number]
#define EXTERN_DECL(decl) extern decl

#define _START_TICKS(prefix)				TYPE_DECL(TICKS_TYPE, IDENTIFIER(prefix, _start_ticks))
#define _END_TICKS(prefix)					TYPE_DECL(TICKS_TYPE, IDENTIFIER(prefix, _end_ticks))
#define _CURRENT_ELAPSED(prefix)		TYPE_DECL(TICKS_TYPE, IDENTIFIER(prefix, _current_elapsed))

#define _ELAPSED(prefix)						ARRAY_DECL(ARRAY_DECL(TYPE_DECL(SAMPLES_TYPE, IDENTIFIER(prefix, _elapsed)), SAMPLES_MAX), NPROCESSORS)

#define _N_ALL(prefix)							ARRAY_DECL(TYPE_DECL(SAMPLES_TYPE, IDENTIFIER(prefix, _n_all)), NPROCESSORS)
#define _N_SUCCESS(prefix)					ARRAY_DECL(TYPE_DECL(SAMPLES_TYPE, IDENTIFIER(prefix, _n_success)), NPROCESSORS)
#define _N_FAIL(prefix)							ARRAY_DECL(TYPE_DECL(SAMPLES_TYPE, IDENTIFIER(prefix, _n_fail)), NPROCESSORS)

#define MEASURE_VARIABLE(prefix) \
	_ELAPSED(prefix);

#define SUCCESS_COUNTER(prefix)		_N_SUCCESS(prefix);
#define FAIL_COUNTER(prefix)			_N_FAIL(prefix);
#define ALL_COUNTER(prefix)				_N_ALL(prefix);

#define EXTERN_MEASURE_VARIABLE(prefix) \
	EXTERN_DECL(_ELAPSED(prefix));

#define MEASURE_STREAM_OPEN(prefix) FILE *out_##prefix = fopen(#prefix, "w")

#define MEASURE_STREAM_CLOSE(prefix)	fclose(out_##prefix)

#define MEASURE_START(variable, cpu) \
	_START_TICKS(variable); \
	_END_TICKS(variable); \
	_CURRENT_ELAPSED(variable); \
	__sync_synchronize(); \
	IDENTIFIER(variable, _start_ticks) = get_ticks(); \
	__sync_synchronize();

#define MEASURE_END(variable, cpu) \
	__sync_synchronize(); \
	IDENTIFIER(variable, _end_ticks) = get_ticks(); \
	__sync_synchronize(); \
	IDENTIFIER(variable, _current_elapsed) = get_elapsed_ticks(IDENTIFIER(variable, _start_ticks), IDENTIFIER(variable, _end_ticks)); \
	if(IDENTIFIER(variable, _current_elapsed) > (TICKS_TYPE)(pow(2, sizeof(SAMPLES_TYPE) * 8) - 1)) \
		fprintf(stderr, "WARNING: sample value too big to be stored in a SAMPLES_TYPE variable\n"); \
	SAMPLES_TYPE sample_num_##variable = IDENTIFIER(variable, _n_all[cpu]); \
	IDENTIFIER(variable, _elapsed[sample_num_##variable][cpu]) = (SAMPLES_TYPE)IDENTIFIER(variable, _current_elapsed); \
	IDENTIFIER(variable, _n_all[cpu])++; \
	if(IDENTIFIER(variable, _n_all[cpu]) > SAMPLES_MAX){ \
		fprintf(stderr, "WARNING: number of samples recorded exceeded SAMPLES_MAX, starting to overwriting first samples...\n"); \
		IDENTIFIER(variable, _n_all[cpu]) = 0; \
	}

#define REGISTER_OUTCOME(variable, cpu, result, bad_value) \
	if(result != bad_value) \
		IDENTIFIER(variable, _n_success[cpu])++; \
	else \
		IDENTIFIER(variable, _n_fail[cpu])++;

#define MEASURE_PRINT(out, variable, cpu) measure_print(out, #variable, cpu, IDENTIFIER(variable, _elapsed), IDENTIFIER(variable, _n_all))

#define OUTCOME_PRINT(out, variable, cpu) outcome_print(out, #variable, cpu, IDENTIFIER(variable, _n_success), IDENTIFIER(variable, _n_fail))

#ifdef MEASURE_CYCLE
	EXTERN_MEASURE_VARIABLE(cycle)
	EXTERN_DECL(ALL_COUNTER(cycle))
#endif

#ifdef MEASURE_SLEEP
	EXTERN_MEASURE_VARIABLE(sleep)
	EXTERN_DECL(ALL_COUNTER(sleep))
#endif

#ifdef MEASURE_PUSH_FIND
	EXTERN_MEASURE_VARIABLE(push_find)
	EXTERN_DECL(SUCCESS_COUNTER(push_find))
	EXTERN_DECL(FAIL_COUNTER(push_find))
	EXTERN_DECL(ALL_COUNTER(push_find))
#endif

#ifdef MEASURE_PULL_FIND
	EXTERN_MEASURE_VARIABLE(pull_find)
	EXTERN_DECL(SUCCESS_COUNTER(pull_find))
	EXTERN_DECL(FAIL_COUNTER(pull_find))
	EXTERN_DECL(ALL_COUNTER(pull_find))
#endif

#ifdef MEASURE_PUSH_PREEMPT
	EXTERN_MEASURE_VARIABLE(push_preempt)
	EXTERN_DECL(ALL_COUNTER(push_preempt))
#endif

#ifdef MEASURE_PULL_PREEMPT
	EXTERN_MEASURE_VARIABLE(pull_preempt)
	EXTERN_DECL(ALL_COUNTER(pull_preempt))
#endif

/* TSC measurement interface */

void set_tsc_cost();
TICKS_TYPE get_tsc_cost();
TICKS_TYPE get_ticks();
TICKS_TYPE get_elapsed_ticks(const TICKS_TYPE start, const TICKS_TYPE end);
TICKS_TYPE ticks_to_seconds(const TICKS_TYPE ticks);
TICKS_TYPE ticks_to_milliseconds(const TICKS_TYPE ticks);
TICKS_TYPE ticks_to_microseconds(const TICKS_TYPE ticks);

/* clock_gettime() measurement interface */

void get_current_thread_time(struct timespec *t);
void get_current_process_time();
struct timespec get_elapsed_time(const struct timespec start, const struct timespec end);

/* measurement print interface */

void measure_print(FILE *out, char *variable_name, int cpu, SAMPLES_TYPE elapsed[][NPROCESSORS], SAMPLES_TYPE *n_all);
void outcome_print(FILE *out, char *variable_name, int cpu, SAMPLES_TYPE *n_success, SAMPLES_TYPE *n_fail);

#endif
