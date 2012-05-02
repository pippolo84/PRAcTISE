#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAMELEN		100
#define N_CPUS_EXP	8
#define N_SCHED_EXP	2

int main(int argc, char *argv[])
{
	FILE *in[N_CPUS_EXP][N_SCHED_EXP];
	FILE *out;
	char filename[N_CPUS_EXP][N_SCHED_EXP][NAMELEN];
	int sample;
	int cpus_num[N_CPUS_EXP] = {2, 4, 8, 16, 24, 32, 40, 48};
	int i, j;
	int flag = 0;

	memset(filename, 0, NAMELEN * N_CPUS_EXP * N_SCHED_EXP);

	if(argc < 4)
		exit(0);

	for(i = 0; i < N_CPUS_EXP; i++){
		sprintf(filename[i][0], "out_%s_%d_0.%d", argv[1], cpus_num[i], atoi(argv[3]));
		in[i][0] = fopen(filename[i][0], "r");
		sprintf(filename[i][1], "out_%s_%d_0.%d", argv[2], cpus_num[i], atoi(argv[3]));
		in[i][1] = fopen(filename[i][1], "r");
	}

	out = fopen("results", "w");

	while(1){
		for(i = 0; i < N_CPUS_EXP; i++){
			for(j = 0; j < N_SCHED_EXP; j++){
				if(fscanf(in[i][j], "%d", &sample) != 1){
					if(i == N_CPUS_EXP - 1 && j == N_SCHED_EXP - 1){
						flag = 1;
						break;
					} else
						fprintf(out, "       ");
				} else
					fprintf(out, "%7d", sample);
				fprintf(out, "\t");
			}
			if(i == N_CPUS_EXP - 1)
				fprintf(out, "\n");
			else
				fprintf(out, "\t");
		}
		if(flag)
			break;
	}

	fclose(out);

	for(i = 0; i < N_CPUS_EXP; i++)
		for(j = 0; j < N_SCHED_EXP; j++)
			fclose(in[i][j]);

	return 0;
}
