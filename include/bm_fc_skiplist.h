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

#ifndef __BM_FC_SKIPLIST_H
#define __BM_FC_SKIPLIST_H

#include <stdio.h>
#include <inttypes.h>
#include <linux/types.h>

typedef struct fc_sl{
	/*
	 * skiplist head doesn't contain any information
	 * but it's required to immediately obtain
	 * the first key inside data structure: head->next[0]
	 */
	struct fc_sl_node *head;
	/*
	 * array that maps a CPU index
	 * to a skiplist node
	 */
	struct fc_sl_node **cpu_to_node;
  /* pseudo-random generation seed */
	unsigned int seed;
	/* actual higher skiplist level */
	unsigned int level;
	/* skiplist elements number */
	unsigned int cpu_num;
	/* compare function */
	int (*cmp_dl)(__u64 a, __u64 b);	
	/* best CPU cache */
	int cached_cpu;

	/* flat combining structure */
	struct flat_combining *fc;
} fc_sl_t;

void fc_sl_init(void *s, int nproc, int (*cmp_dl)(__u64 a, __u64 b));
void fc_sl_cleanup(void *s);
/*
 * Update CPU state inside the 
 * data structure
 */
int fc_sl_preempt(void *s, int cpu, __u64 dline, int is_valid);
/*
 * data_find should find the best CPU where to push
 * a task and/or find the best task to pull from
 * another CPU
 */
int fc_sl_find(void *s);

void fc_sl_load(void *s, FILE *f);
void fc_sl_save(void *s, int nproc, FILE *f);
void fc_sl_print(void *s, int nproc);

int fc_sl_check(void *s, int nproc);
int fc_sl_check_cpu(void *s, int cpu, __u64 dline);

#endif
