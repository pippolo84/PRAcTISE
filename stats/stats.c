#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#define TRANSIENT_RATE		0.1

#define BUFLEN						100

//#define DEBUG

void usage(const char *name);
void remove_transient_samples(long long unsigned **samples, long long unsigned *total_number, const int cpus);
long long unsigned samples_avg(long long unsigned *samples, const long long unsigned total_number);
long double percentile_index(const long long unsigned total_number, const float percentage);
long double percentile_value(long long unsigned *samples, const long double index, const long long unsigned total_number);
void make_stats(FILE *in);
void print_all(int cpus, long long unsigned *total_number, long long unsigned **samples, long long unsigned *success_ops, long long unsigned *fail_ops);

int compare(const void *a, const void *b);

int main(int argc, char *argv[])
{
	FILE *in;	

	if(argc < 2){
		usage(argv[0]);
		exit(0);
	}

	in = fopen(argv[1], "r");
	if(!in){
		fprintf(stderr, "fopen: %s\n", strerror(errno));
		exit(-1);
	}

	make_stats(in);

	fclose(in);

	return 0;
}

void usage(const char *name)
{
	printf("usage: %s file_name\n", name);
}

void remove_transient_samples(long long unsigned **samples, long long unsigned *total_number, const int cpus)
{
	int i;
	long long unsigned invalid;

	for(i = 0; i < cpus; i++){
		invalid = (long long unsigned)(total_number[i] * TRANSIENT_RATE);	
		memcpy(&samples[i][0], &samples[i][invalid], (total_number[i] - invalid) * sizeof(**samples));
		total_number[i] -= invalid;
	}
}

long long unsigned samples_avg(long long unsigned *samples, const long long unsigned total_number)
{
	long long unsigned sum = 0;
	int i;

	for(i = 0; i < total_number; i++)
		sum += samples[i];

	return sum / total_number;
}

long double percentile_index(const long long unsigned total_number, const float percentage)
{
	long double quantile = percentage;

	return (total_number + 1) * quantile - 1;
}

long double percentile_value(long long unsigned *samples, const long double index, const long long unsigned total_number)
{
	long long unsigned prev, next;
	long double value;
	
	prev = (long long unsigned int)floorl(index);
	next = (long long unsigned int)ceill(index);
	if(next >= total_number)
		return (long double)samples[prev];

	/* linear interpolation */
	value = (long double)samples[prev] + (index - floorl(index)) * ((double)samples[next] - samples[prev]);
	
	return value;
}

void make_stats(FILE *in)
{
	int cpus;
	long long unsigned *total_number;
	long long unsigned **samples;
	long long unsigned *success_ops;
	long long unsigned *fail_ops;
	int i, j;
	char buffer[BUFLEN];
	int ch, outcome_flag;
	long double index;

	outcome_flag = 0;

	/* read CPUs number */
	fscanf(in, "CPUs number:\t%d\n", &cpus);

	total_number = (long long unsigned *)calloc(cpus, sizeof(*total_number));
	if(!total_number){
		fprintf(stderr, "calloc: %s\n", strerror(errno));
		exit(-1);
	}
	success_ops = (long long unsigned *)calloc(cpus, sizeof(*success_ops));
	if(!success_ops){
		fprintf(stderr, "calloc: %s\n", strerror(errno));
		exit(-1);
	}
	fail_ops = (long long unsigned *)calloc(cpus, sizeof(*fail_ops));
	if(!fail_ops){
		fprintf(stderr, "calloc: %s\n", strerror(errno));
		exit(-1);
	}
	samples = (long long unsigned **)calloc(cpus, sizeof(*samples));
	if(!samples){
		fprintf(stderr, "calloc: %s\n", strerror(errno));
		exit(-1);
	}

	for(i = 0; i < cpus; i++){
		fgets(buffer, BUFLEN, in);

		/* read total number */
		fscanf(in, "total number:\t%llu\n", &total_number[i]);
		if(total_number[i]){
			samples[i] = (long long unsigned *)calloc(total_number[i], sizeof(*samples[i]));
			if(!samples[i]){
				fprintf(stderr, "calloc: %s\n", strerror(errno));
				exit(-1);
			}
		}

		/* read samples */
		for(j = 0; j < total_number[i]; j++)
			fscanf(in, "%llu\n", &samples[i][j]);

		fscanf(in, "[%d]", &ch);
		if(ch == i){
			/* read outcome */
			outcome_flag = 1;
			fgets(buffer, BUFLEN, in);
			fscanf(in, "%s %s %llu\n", buffer, buffer, &success_ops[i]);
			fscanf(in, "%s %s %llu\n", buffer, buffer, &fail_ops[i]);
		}
	}

#ifdef DEBUG
	print_all(cpus, total_number, samples, success_ops, fail_ops);
#endif

	/* remove transient samples */
	remove_transient_samples(samples, total_number, cpus);

	/* sort samples */
	for(i = 0; i < cpus; i++)
		qsort(&samples[i][0], total_number[i], sizeof(**samples), compare);

	for(i = 0; i < cpus; i++){
		printf("CPU %d\n", i);

		/* min max avg */
		printf("min:\t\t\t%llu\n", samples[i][0]);
		printf("max:\t\t\t%llu\n", samples[i][total_number[i] - 1]);
		printf("avg:\t\t\t%llu\n", samples_avg(&samples[i][0], total_number[i]));

		/* outcome */
		if(outcome_flag){
			printf("success:\t\t%llu\n", success_ops[i]);
			printf("fail:\t\t\t%llu\n", fail_ops[i]);
		}

		/* percentiles */
		index = percentile_index(total_number[i], 0.70);
		printf("percentile(70%%):\t%.0Lf\n", roundl(percentile_value(&samples[i][0], index, total_number[i])));

		index = percentile_index(total_number[i], 0.85);
		printf("percentile(85%%):\t%.0Lf\n", roundl(percentile_value(&samples[i][0], index, total_number[i])));

		index = percentile_index(total_number[i], 0.95);
		printf("percentile(95%%):\t%.0Lf\n", roundl(percentile_value(&samples[i][0], index, total_number[i])));

		printf("\n");
	}
	
#ifdef DEBUG
	print_all(cpus, total_number, samples, success_ops, fail_ops);
#endif

	for(i = 0; i < cpus; i++)
		free(samples[i]);
	free(samples);
	free(fail_ops);
	free(success_ops);
	free(total_number);
}

int compare(const void *a, const void *b)
{
	long long unsigned *first, *second;

	first = (long long unsigned *)a;
	second = (long long unsigned *)b;

	return *first > *second;
}

void print_all(int cpus, long long unsigned *total_number, long long unsigned **samples, long long unsigned *success_ops, long long unsigned *fail_ops)
{
	int i, j;

	printf("cpus: %d\n\n", cpus);
	for(i = 0; i < cpus; i++){
		printf("[%d]:\ttotal number: %llu\n", i, total_number[i]);
		for(j = 0; j < total_number[i]; j++)
			printf("[%d]:\t%llu\n", i, samples[i][j]);
		printf("[%d]:\tsuccess ops: %llu\n", i, success_ops[i]);
		printf("[%d]:\tfail ops: %llu\n", i, fail_ops[i]);
		printf("\n");
	}
}
