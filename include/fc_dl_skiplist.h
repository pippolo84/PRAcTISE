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

#ifndef __FC_DL_SKIPLIST_H
#define __FC_DL_SKIPLIST_H

#include <stdio.h>
#include <inttypes.h>
#include <linux/types.h>
#include <pthread.h>

#include "common_ops.h"
#include "flat_combining.h"

/* doubly-linked skiplist */
typedef struct _fc_dl_skiplist {
	struct fc_dl_sl *list;
	int (*cmp_dl)(__u64 a, __u64 b);
  pthread_spinlock_t lock;
  pub_list *p_list;
  pub_record ***p_record_array;
	/* array di indici dell'ultimo publication record utilizzato */
  int *p_record_idx;
} fc_dl_skiplist_t;

struct fc_dl_sl{
	/*
	 * La testa della skip list è un nodo di struttura 
	 * che non contiene informazione.
	 * Permette inoltre di trovare immediatamente il nodo avente
	 * la chiave più piccola: head->next[0]
	 */
	struct fc_dl_sl_node *head;
	/*
	 * L'array implementa il mapping tra le runqueue, identificate
 	 * con il loro indice, e i nodi della skiplist.
	 */
	struct fc_dl_sl_node **rq_to_node;
  /* Seme per la generazione di numeri pseudo-random */
	unsigned int seed;
	/* Livello attuale della skiplist */
	unsigned int level;
	/* Dimensione della skiplist */
	unsigned int rq_num;
};

void fc_dl_sl_init(void *s, int nproc, int (*cmp_dl)(__u64 a, __u64 b));
void fc_dl_sl_cleanup(void *s);

/*
 * Update CPU state inside the data structure
 * after a preemption
 */
int fc_dl_sl_preempt(void *s, int cpu, __u64 dline, int is_valid);
/*
 * Update CPU state inside the data structure
 * after a task finished 
 */
int fc_dl_sl_finish(void *s, int cpu, __u64 dline, int is_valid);
/*
 * data_find should find the best CPU where to push
 * a task and/or find the best task to pull from
 * another CPU
 */
int fc_dl_sl_find(void *s);
int fc_dl_sl_max(void *s);

void fc_dl_sl_load(void *s, FILE *f);
void fc_dl_sl_save(void *s, int nproc, FILE *f);
void fc_dl_sl_print(void *s, int nproc);

int fc_dl_sl_check(void *s, int nproc);
int fc_dl_sl_check_cpu(void *s, int cpu, __u64 dline);

#endif
