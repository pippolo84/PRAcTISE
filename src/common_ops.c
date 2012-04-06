#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "common_ops.h"
#include "rq_heap.h"
#include "measure.h"
#include "parameters.h"

#define PUSH_MAX_TRIES		3
#define PULL_MAX_TRIES		3

/*
 * With this source file we implement a
 * runqueue based on a binomial heap.
 * Since we have some discrepancies from
 * Linux scheduler, we indicate these
 * with a comment like "in Linux..."
 */

/*
 * __dl_time_before - compare two deadlines, return > 0 if 
 * the first is earlier than the second one
 * @a:		first deadline
 * @b:		second deadline
 */
int __dl_time_before(__u64 a, __u64 b)
{
	return (__s64)(a - b) < 0;
}

/*
 * __dl_time_after - compare two deadlines, return > 0 if 
 * the first is later than or equal to the second one
 * @a:		first deadline
 * @b:		second deadline
 */
int __dl_time_after(__u64 a, __u64 b)
{
	return (__s64)(a - b) > 0;
}

/*
 * task_compare - compare the deadlines of two struct rq_heap_node,
 * return > 0 if the first is earlier than the second one
 * @a:		pointer to first struct rq_heap_node
 * @b:		pointer to second struct rq_heap_node
 */
static int task_compare(struct rq_heap_node* _a, struct rq_heap_node* _b)
{
	struct task_struct *a, *b;

	if(!_a || !_b){
#ifdef DEBUG	
		fprintf(stderr, "ERROR: passing NULL pointer to task_compare!\n");
#endif
		exit(1);
	}

	a = (struct task_struct*) rq_heap_node_value(_a);
	b = (struct task_struct*) rq_heap_node_value(_b);

	if(!a || !b){
#ifdef DEBUG	
		fprintf(stderr, "ERROR: passing NULL pointer to __dl_time_before!\n");
#endif
		exit(1);
	}

	return __dl_time_before(a->deadline, b->deadline);
}

/*
 * rq_init - initialize the runqueue structure
 * @rq:		pointer to struct rq we want to initialize
 * @cpu:	index of CPU bounded to runqueue
 * @f:		stream log 
 */
void rq_init (struct rq *rq, int cpu, FILE *f)
{
	rq->cpu = cpu;
	rq_heap_init(&rq->heap);
	pthread_spin_init(&rq->lock, 0);
	rq->earliest = 0;
	rq->next = 0;
	rq->nrunning = 0;
	rq->overloaded = 0;
	rq->log = f;
}

/*
 * rq_destroy - destroy the runqueue structure
 * @rq:		pointer to struct rq we want to destroy
 */
void rq_destroy (struct rq *rq)
{
	struct rq_heap_node *node;

	while(!rq_heap_empty(&rq->heap)){
		node = rq_heap_take(task_compare, &rq->heap);
		free((struct task_struct *)rq_heap_node_value(node));
		free(node);
	}
}

/*
 * rq_lock - acquire the runqueue's lock
 * @rq:		the runqueue to lock
 */
void rq_lock (struct rq *rq)
{
	if(pthread_spin_lock(&rq->lock)){
#ifdef DEBUG
		fprintf(stderr, "error while acquiring spin lock on runqueue %d\n", rq->cpu);
#endif
		exit(1);	
	}
}

/*
 * rq_unlock - release the runqueue's lock
 * @rq:		the runqueue to unlock
 */
void rq_unlock (struct rq *rq)
{
	if(pthread_spin_unlock(&rq->lock)){
#ifdef DEBUG
		fprintf(stderr, "error while releasing spin lock on runqueue %d\n", rq->cpu);
#endif
		exit(1);
	}
}

/*
 * rq_double_lock - acquire the locks on two runqueues in
 * a deadlock-free (but unfair) manner: we always acquire
 * the lower id CPU runqueue lock first
 * @rq1:		pointer to first runqueue
 * @rq2:		pointer to second runqueue
 *
 */
static void rq_double_lock(struct rq *rq1, struct rq *rq2){
	/* 
	 * rq1 and rq2 are the same and
	 * we already have the lock on rq1
	 */
	if(rq1->cpu == rq2->cpu){
#ifdef DEBUG
		fprintf(stderr, "WARNING: trying to acquire double lock on same runqueue: rq #%d\n", rq1->cpu);
#endif
		return;
	}

	/* 
	 * rq1 is the lower id CPU runqueue
	 * than we acquire the lock on rq2
	 */
	if(rq1->cpu - rq2->cpu < 0){
		rq_lock(rq2);

		return;
	}

	/* 
	 * rq2 is the lower id CPU runqueue
	 * than we have to:
	 * release the lock on rq1
	 * acquire the lock on rq2 and on rq1
	 * (in that order)
	 */
	rq_unlock(rq1);
	rq_lock(rq2);
	rq_lock(rq1);
}

/*
 * rq_peek - peek the runqueue for the earliest deadline task,
 * return a pointer to the task if runqueue is not empty, NULL otherwise
 * @rq:		the runqueue we want to peek at
 */
struct rq_heap_node *rq_peek (struct rq *rq)
{
	return rq_heap_peek(task_compare, &rq->heap);
}

/*
 * rq_take - remove the earliest deadline task in the runqueue,
 * return a pointer to that task if runqueue is not empty, NULL otherwise
 * @rq: the runqueue we want to take the task from
 */
struct rq_heap_node *rq_take (struct rq *rq)
{
	struct rq_heap_node  *ns_taken, *ns_next;
	struct task_struct* ts_next;
	int is_valid;

	if (rq->nrunning < 1) {
#ifdef DEBUG
		fprintf(rq->log, "[%d] ERROR: dequeue on an empty queue!\n", rq->cpu);
#endif
		exit(-1);
	}

	if (--rq->nrunning < 2)
		rq->overloaded = 0;

	ns_taken = rq_heap_take(task_compare, &rq->heap);

	/* earliest cache update */
	rq->earliest = rq->next;
	/* update push global data structure */
	is_valid = rq->earliest != 0 ? 1 : 0;
#ifdef MEASURE_PUSH_PREEMPT
	MEASURE_START(push_preempt, rq->cpu)
#endif
	dso->data_preempt(push_data_struct, rq->cpu, rq->earliest, is_valid);
#ifdef MEASURE_PUSH_PREEMPT
	MEASURE_END(push_preempt, rq->cpu)
#endif
	/* next cache update */
	ns_next = rq_heap_peek_next(task_compare, &rq->heap);
	if (ns_next != NULL) {
		ts_next = (struct task_struct*) rq_heap_node_value(ns_next);
		rq->next = ts_next->deadline;
	} else
		rq->next = 0;
	/* update pull global data structure */
	is_valid = rq->next != 0 ? 1 : 0;
#ifdef MEASURE_PULL_PREEMPT
	MEASURE_START(pull_preempt, rq->cpu)
#endif
	dso->data_preempt(pull_data_struct, rq->cpu, rq->next, is_valid);
#ifdef MEASURE_PULL_PREEMPT
	MEASURE_END(pull_preempt, rq->cpu)
#endif

	return ns_taken;
}

/*
 * rq_take_next - remove the next earliest deadline task in the runqueue,
 * return a pointer to that task if runqueue has two or more deadline task
 * in it, NULL otherwise
 * @rq:		the runqueue we want to take the task from
 */
struct rq_heap_node *rq_take_next (struct rq *rq)
{
	struct rq_heap_node *ns_next, *new_ns_next;
	struct task_struct* new_ts_next;
	int is_valid;

	if (rq->nrunning < 2) {
#ifdef DEBUG
		fprintf(rq->log, "[%d] ERROR: dequeue next on a not overloaded queue!\n", rq->cpu);
		rq_print(rq, stderr);
#endif
		exit(-1);
	}

	if (--rq->nrunning < 2)
		rq->overloaded = 0;

	ns_next = rq_heap_take_next(task_compare, &rq->heap);

	/* next cache update */
	if (ns_next != NULL && (new_ns_next = rq_heap_peek_next(task_compare, &rq->heap))) {
		new_ts_next = (struct task_struct *)rq_heap_node_value(new_ns_next);
		rq->next = new_ts_next->deadline;
	} else
		rq->next = 0;
	/* update pull global data structure */
	is_valid = rq->next != 0 ? 1 : 0;
#ifdef MEASURE_PULL_PREEMPT
	MEASURE_START(pull_preempt, rq->cpu)
#endif
	dso->data_preempt(pull_data_struct, rq->cpu, rq->next, is_valid);
#ifdef MEASURE_PULL_PREEMPT
	MEASURE_END(pull_preempt, rq->cpu)
#endif

	return ns_next;
}

/*
 * add_task_rq - enqueue a task to the runqueue
 * @rq:			the runqueue we want to enqueue the task to
 * @task:		the task we want to enqueue
 */
void add_task_rq(struct rq* rq, struct task_struct* task)
{
	__u64 task_dl = task->deadline;
	__u64 old_earliest = rq->earliest, old_next = rq->next;
	struct rq_heap_node* hn;
	int is_valid;

	hn = calloc(1, sizeof(*hn));
	if(!hn){
		fprintf(stderr, "out of memory\n");
		fflush(stderr);
		sync();
		exit(-1);	
	}

	rq_heap_node_init(hn, task);
	rq_heap_insert(task_compare, &rq->heap, hn);

	/* min and next cache update */
	if (rq->nrunning == 0 || __dl_time_before(task_dl, old_earliest)) {
		rq->next = old_earliest;
		rq->earliest = task_dl;
		/* update push global data structure */
		is_valid = rq->earliest != 0 ? 1 : 0;
#ifdef MEASURE_PUSH_PREEMPT
		MEASURE_START(push_preempt, rq->cpu)
#endif
		dso->data_preempt(push_data_struct, rq->cpu, rq->earliest, is_valid);
#ifdef MEASURE_PUSH_PREEMPT
		MEASURE_END(push_preempt, rq->cpu)
#endif
		/* update pull global data structure */
		is_valid = rq->next != 0 ? 1 : 0;
#ifdef MEASURE_PULL_PREEMPT
		MEASURE_START(pull_preempt, rq->cpu)
#endif
		dso->data_preempt(pull_data_struct, rq->cpu, rq->next, is_valid);
#ifdef MEASURE_PULL_PREEMPT
		MEASURE_END(pull_preempt, rq->cpu)
#endif
	} else if (!rq->overloaded || __dl_time_before(task_dl, old_next)){
		rq->next = task_dl;
		/* update pull global data structure */
		is_valid = rq->next != 0 ? 1 : 0;
#ifdef MEASURE_PULL_PREEMPT
		MEASURE_START(pull_preempt, rq->cpu)
#endif
		dso->data_preempt(pull_data_struct, rq->cpu, rq->next, is_valid);
#ifdef MEASURE_PULL_PREEMPT
		MEASURE_END(pull_preempt, rq->cpu)
#endif
	}

	if (++rq->nrunning > 1)
		rq->overloaded = 1;
}

/*
 * find_earlier_rq - find the runqueue with earliest deadline,
 * return the index of CPU bounded to the runqueue found,
 * -1 if search failed
 * @this_cpu:		id of calling CPU
 */
static int find_earlier_rq(int this_cpu){
	int best_cpu;

#ifdef MEASURE_PULL_FIND
	MEASURE_START(pull_find, this_cpu)
#endif
	best_cpu = dso->data_find(pull_data_struct);
#ifdef MEASURE_PULL_FIND
	MEASURE_END(pull_find, this_cpu)
	REGISTER_OUTCOME(pull_find, this_cpu, best_cpu, -1)
#endif

	return best_cpu;
}

/*
 * find_lock_earlier_rq: search for the runqueue with the earlier 
 * deadline and lock it, together with the destination runqueue,
 * return a pointer to the runqueue, NULL if search fails
 * @this_rq:		pointer to the destination runqueue
 */
static struct rq *find_lock_earlier_rq(struct rq *this_rq){
	struct rq *earlier_rq = NULL;
	struct rq_heap_node *node;
	int tries;
	int cpu;

	for(tries = 0; tries < PULL_MAX_TRIES; tries++) {
		cpu = find_earlier_rq(this_rq->cpu);

		if((cpu == -1) || (cpu == this_rq->cpu))
			break;

		/*
		 * we have to check earlier_rq
		 * 'cause that processor
		 * may have terminated the
		 * simulation and destroyed
		 * his runqueue
		 */
		if(!(earlier_rq = cpu_to_rq[cpu]))
			break;

		/* locks acquire on source and destination runqueues */
		rq_double_lock(this_rq, earlier_rq);

		/* check if the candidate runqueue still has task in */ 
		node = rq_heap_peek_next(task_compare, &earlier_rq->heap);
		if(node)
			break;

		/* retry */
		rq_unlock(earlier_rq);
		earlier_rq = NULL;
	}

	return earlier_rq;
}

/*
 * rq_pull_tasks - try to pull a task from another runqueue
 * @this_rq:		pointer to destination runqueue 
 */
int rq_pull_tasks(struct rq* this_rq)
{
	struct rq_heap_node *node;
	struct task_struct *task;
	struct rq *src_rq;
	
#if 0
	/* 
	 * if runqueue is overloaded we don't pull anything
	 * in Linux this check is done only when we try to pull
	 * from prio_changed() so we don't do anything here 
	 */
	if(this_rq->overloaded)
		return;
#endif

	/* 
	 * ask the global data structure for a suitable runqueue 
	 * to pull from, then lock source and destination runqueue.
	 * In Linux we don't have any global data structure, so
	 * we try to pull from any CPU in the root_domain, until
	 * we can't find a task with a deadline earliest than last
	 * pulled.
	 * Here we pull only one task, hopefully the one who have,
	 * globally, the earliest deadline.
	 */
	src_rq = find_lock_earlier_rq(this_rq);
	if(src_rq){
		/* 
		 * migrate task 
		 */
		node = rq_take_next(src_rq);
		task = rq_heap_node_value(node);
		add_task_rq(this_rq, task);

		rq_unlock(src_rq);
		free(node);

		return 1;
	}

	return 0;
}

/*
 * find_later_rq - find the runqueue with latest deadline,
 * return the index of CPU bounded to the runqueue found,
 * -1 if search failed
 * @task:			the task we want to push
 * @this_cpu:	id of calling CPU
 */
static int find_later_rq(struct task_struct *task, int this_cpu){
	int best_cpu;

	/* 
	 * in Linux there's a idle CPUs mask
	 * and find_later_rq starts to search
	 * from that
	 */
	/*
	 * in Linux we also have to handle
	 * the task CPU affinity
	 */
#ifdef MEASURE_PUSH_FIND
	MEASURE_START(push_find, this_cpu)
#endif
	best_cpu = dso->data_find(push_data_struct);
#ifdef MEASURE_PUSH_FIND
	MEASURE_END(push_find, this_cpu)
	REGISTER_OUTCOME(push_find, this_cpu, best_cpu, -1)
#endif

	return best_cpu;
}

/*
 * find_lock_later_rq - search for the runqueue with the latest 
 * deadline and lock it, together with the source runqueue,
 * return a pointer to the runqueue, NULL if search fails
 * @task:			task we want to push
 * @this_rq:	pointer to the source runqueue
 */
static struct rq* find_lock_later_rq(struct task_struct *task,
		struct rq *this_rq)
{
	struct rq *later_rq = NULL;
	struct rq_heap_node *node;
	int tries;
	int cpu;

	for(tries = 0; tries < PUSH_MAX_TRIES; tries++) {
		cpu = find_later_rq(task, this_rq->cpu);
		
		if((cpu == -1) || (cpu == this_rq->cpu))
			break;

		/*
		 * we have to check later_rq
		 * 'cause that processor
		 * may have terminated the
		 * simulation and destroyed
		 * his runqueue
		 */
		if(!(later_rq = cpu_to_rq[cpu]))
			break;

		/* 
		 * we acquire locks on this_rq
		 * and later_rq, then we check
		 * if something is changed (rq_double_lock
		 * might release this_rq lock for
		 * deadlock avoidance purpose)
		 */
		rq_double_lock(this_rq, later_rq);
		node = rq_heap_peek_next(task_compare, &this_rq->heap);
		if(rq_heap_node_value(node) != task){	/* something changed */
			rq_unlock(later_rq);
			later_rq = NULL;

			break;
		}
		
		/*
		 * check if later_rq actually contains a task
		 * with a later deadline. This is necessary 'cause
		 * in some implementations of the global data structure
		 * we can have a misalignment
		 */
		if(__dl_time_before(task->deadline, later_rq->earliest))
			break;

		/* retry */
		rq_unlock(later_rq);
		later_rq = NULL;
	}

	return later_rq;
}

/*
 * rq_push_task - try to push a task from am overloaded runqueue
 * to another, return 1 if push take place, 0 otherwise
 * @this_rq:	the source runqueue
 */
static int rq_push_task(struct rq* this_rq, int *push_count)
{
	struct rq_heap_node *node;
	struct task_struct *next_task;
	struct rq *later_rq;

	/* if there's nothing to push: return */
	if (!this_rq->overloaded)
		return 0;

	/* catch the next earliest deadline task */
	/*
	 * in Linux we have a pushable tasks RB-tree,
	 * so we take the first task from that tree,
	 * not the next task enqueued
	 */
	node = rq_heap_peek_next(task_compare, &this_rq->heap);
	if (!node){
#ifdef DEBUG
		fprintf(this_rq->log, "[%d] ERROR: runqueue is overloaded but rq_heap_peek_next returns NULL\n", this_rq->cpu);
		rq_print(this_rq, this_rq->log);
		exit(1);
#endif
		return 0;
	}
	next_task = rq_heap_node_value(node);

retry:
	node = rq_heap_peek(task_compare, &this_rq->heap);
	if (next_task == rq_heap_node_value(node)) {
#ifdef DEBUG
		fprintf(this_rq->log, "[%d] WARNING: next_task = min_task inside push\n", this_rq->cpu);
#endif
		return 0;
	}

	/*
	 * if next_task preempts task currently executing
	 * on this_rq we don't go further in pushing next_task
	 */
	if(__dl_time_before(next_task->deadline, this_rq->earliest))
		return 0;

	/*
	 * Will lock the rq it'll find
	 */
	later_rq = find_lock_later_rq(next_task, this_rq);
	if (!later_rq) {
		struct task_struct *task;

		/*
		 * We must check all this again. find_lock_later_rq
		 * releases rq->lock, then it is possible that next_task
		 * has migrated
		 */
		node = rq_heap_peek_next(task_compare, &this_rq->heap);
		task = rq_heap_node_value(node);
		if (task == next_task) {
			/*
			 * The task is still there, we don't try
			 * again.
			 */
			/*
			 * in Linux:
			 * 1) dequeue_pushable_task()
			 * 2) goto out
			 * since here we have no pushable task RB-tree,
			 * we simply stop to push and return 0
			 */
			return 0;
		}

		/*
		 * no more tasks
		 */
		if (!task)
			goto out;

		/* retry */
		next_task = task;
		goto retry;
	}

	/*
	 * migrate task
	 */
	node = rq_take_next(this_rq);
	next_task = rq_heap_node_value(node);
	add_task_rq(later_rq, next_task);

	(*push_count)++;

	rq_unlock(later_rq);
	free(node);

out:
	return 1;
}

/*
 * rq_push_tasks: keep pushing tasks as it
 * fails to move one. For performance measurements
 * purpose we account the number of successfully
 * pushed tasks.
 * @this_rq:	the runqueue we want to push task from
 */
int rq_push_tasks(struct rq* this_rq)
{
	int push_count = 0;

	/* Terminates as is fails to move a task */
	while (rq_push_task(this_rq, &push_count))
		;

	return push_count;
}

/**************************************
*		Some useful debugging functions		*
**************************************/

/*
 * task_print - print info about a task
 * @task:		a pointer to struct task_struct
 * @out:		output stream
 */
static void task_print(struct task_struct *task, FILE *out){
	fprintf(out, "\tpid: %d deadline: %llu\n", task->pid, task->deadline);
}

/*
 * rq_heap_print_recursive - print recursively binomial heap nodes
 * @node:		a pointer to a heap node we want to print
 * @out:		output stream
 */
static void rq_heap_print_recursive(struct rq_heap_node *node, FILE *out){
	if(!node)
		return;

	/* print node value */
	task_print((struct task_struct *)node->value, out);

	if(!node->parent){			/* binomial tree root */
		rq_heap_print_recursive(node->child, out);
		rq_heap_print_recursive(node->next, out);
	}else{
		rq_heap_print_recursive(node->next, out);
		rq_heap_print_recursive(node->child, out);
	}
}

/*
 * rq_check - check runqueue correctness
 * @rq:		the runqueue we want to check
 */
int rq_check(struct rq *rq){
	struct rq_heap *rq_to_check;
	struct rq_heap rq_backup;
	struct rq_heap_node *min, *next, *node;
	int flag = 1;

	if(!rq)
		return 0;

	if(!rq->earliest && rq->next)
		flag = 0;
	if(rq->next && rq->earliest && __dl_time_before(rq->next, rq->earliest))
		flag = 0;
	if(rq->nrunning < 2 && rq->overloaded)
		flag = 0;

	rq_to_check = &rq->heap;

	if(!rq->earliest && !rq->next && !rq_heap_empty(rq_to_check))
		flag = 0;

	/* 
	 * initialize a backup runqueue where
	 * we save extracted node
	 */
	rq_heap_init(&rq_backup);

	next = rq_heap_take_next(task_compare, rq_to_check);
	min = rq_heap_take(task_compare, rq_to_check);

	if(min && next && task_compare(next, min))
		flag = 0;
	if(!min && !next && (!rq_heap_empty(rq_to_check)))
		flag = 0;

	if(min)
		rq_heap_insert(task_compare, &rq_backup, min);
	if(next)
		rq_heap_insert(task_compare, &rq_backup, next);

	while((node = rq_heap_take(task_compare, rq_to_check))){
		if(task_compare(node, min) ||	task_compare(node, next))
			flag = 0;

		rq_heap_insert(task_compare, &rq_backup, node);
	}

	/* restore checked runqueue */
	while((node = rq_heap_take(task_compare, &rq_backup)))
		rq_heap_insert(task_compare, rq_to_check, node);

	return flag;
}

/*
 * rq_print - print current runqueue state
 * @this_rq:		a pointer to the runqueue we want to print
 * @out:				output stream
 */
void rq_print(struct rq *this_rq, FILE *out){
	if(!this_rq)
		return;

	fprintf(out, "\n");

	fprintf(out, "----runqueue %d----\n", this_rq->cpu);

	fprintf(out, "nrunning: %d, overloaded: %d\n", this_rq->nrunning, this_rq->overloaded);
	fprintf(out, "cached value --> earliest: %llu, next: %llu\n", this_rq->earliest, this_rq->next);

	if(this_rq->heap.min){
		fprintf(out, "min cached node:\n");
		task_print((struct task_struct *)this_rq->heap.min->value, out);
	}
	if(this_rq->heap.next){
		fprintf(out, "next cached node:\n");
		task_print((struct task_struct *)this_rq->heap.next->value, out);
	}
	
	fprintf(out, "nodes in binomial heap:\n");
	rq_heap_print_recursive(this_rq->heap.head, out);

	fprintf(out, "----end runqueue %d----\n\n", this_rq->cpu);
}