#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define MAX_LINE	100
#define MAX_SAMPLES	48

int main(int argc, char *argv[])
{
	char line[MAX_LINE];
	int rate[MAX_SAMPLES];
	int index, sample;
	char useless[MAX_LINE];
	int i;
	long long unsigned sum;
	FILE *in;

	if(argc < 2)
		exit(0);

	if(!(in = fopen(argv[1], "r"))){
		fprintf(stderr, "fopen:\t%s\n", strerror(errno));
		exit(1);
	}

	memset(rate, 0, sizeof(*rate) * MAX_SAMPLES);

	while(fgets(line, MAX_LINE, in)){
		if(strstr(line, "rate"))
			if(sscanf(line, "[%d]: %s rate: %d event/s", &index, useless, &sample) != 3){
				fprintf(stderr, "sscanf:\t%s\n", strerror(errno));
				exit(1);
			}
			rate[index] = sample;
	}
	for(i = 0, sum = 0ULL; i <= index; i++)
		sum += rate[i];

	printf("%llu\n", sum);

	fclose(in);

	return 0;
}
