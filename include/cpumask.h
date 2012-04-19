#ifndef __CPUMASK_H
#define __CPUMASK_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "parameters.h"

#ifdef SCHED_RT

/* from include/linux/sched.h */
#define tsk_cpus_allowed(tsk) (&(tsk)->cpus_allowed)

/* from include/linux/bitops.h */
#define BITS_PER_BYTE					8

#ifdef __i386__ 
	#define BITS_PER_LONG					(4 * BITS_PER_BYTE)
#elif __x86_64__
	#define BITS_PER_LONG					(8 * BITS_PER_BYTE)
#endif

/* from /tools/perf/util/include/linux/bitops.h */
static inline void set_bit(int nr, unsigned long *addr)
{
	__sync_fetch_and_or(&addr[nr / BITS_PER_LONG], 1UL << (nr % BITS_PER_LONG));
}

static inline void clear_bit(int nr, unsigned long *addr)
{
	__sync_fetch_and_and(&addr[nr / BITS_PER_LONG], ~(1UL << (nr % BITS_PER_LONG)));
}

static __always_inline int test_bit(unsigned int nr, const unsigned long *addr)
{
	return ((1UL << (nr % BITS_PER_LONG)) &
		(((unsigned long *)addr)[nr / BITS_PER_LONG])) != 0;
}

/**
 * from /include/asm-generic/bitops/__ffs.h
 *
 * __ffs - find first set bit in word
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
static __always_inline unsigned long __ffs(unsigned long word)
{
	int num = 0;

#if BITS_PER_LONG == 64
	if((word & 0xffffffff) == 0) {
		num += 32;
		word >>= 32;
	}
#endif
	if((word & 0xffff) == 0) {
		num += 16;
		word >>= 16;
	}
	if((word & 0xff) == 0) {
		num += 8;
		word >>= 8;
	}
	if((word & 0xf) == 0) {
		num += 4;
		word >>= 4;
	}
	if((word & 0x3) == 0) {
		num += 2;
		word >>= 2;
	}
	if((word & 0x1) == 0)
		num += 1;
	return num;
}

/* from /lib/find_next_bit.c */
unsigned long find_next_zero_bit(const unsigned long *addr, unsigned long size,
                                 unsigned long offset);
unsigned long find_next_bit(const unsigned long *addr, unsigned long size,
														unsigned long offset);
unsigned long find_first_bit(const unsigned long *addr, unsigned long size);

/**
 * from include/linux/kernel.h
 *
 * for more informations about number
 * conversion visit:
 * http://www.cs.nott.ac.uk/~rcb/G51MPC/slides/NumberLogic.pdf
 */
#define DIV_ROUND_UP(n, d)		(((n) + (d) - 1) / (d))

/* from include/linux/bitops.h */
#define BITS_TO_LONGS(nr)			DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

/* from /include/linux/bitmap.h */
#define BITMAP_LAST_WORD_MASK(nbits)															\
(                             																		\
         ((nbits) % BITS_PER_LONG) ?															\
                 (1UL << ((nbits) % BITS_PER_LONG)) - 1 : ~0UL		\
)

static inline void bitmap_zero(unsigned long *dst, int nbits)
{
	int len = BITS_TO_LONGS(nbits) * sizeof(unsigned long);
	memset(dst, 0, len);
}

static inline void bitmap_fill(unsigned long *dst, int nbits)
{
	int nlongs = BITS_TO_LONGS(nbits);
	int len = (nlongs - 1) * sizeof(unsigned long);
	memset(dst, 0xff, len);
	dst[nlongs - 1] = BITMAP_LAST_WORD_MASK(nbits);
}

static inline int bitmap_and(unsigned long *dst, const unsigned long *src1,
														const unsigned long *src2, int nbits)
{
	return __bitmap_and(dst, src1, src2, nbits);
}

/* from include/linux/types.h */
#define DECLARE_BITMAP(name, bits) \
	unsigned long name[BITS_TO_LONGS(bits)]

typedef struct cpumask { DECLARE_BITMAP(bits, NR_CPUS); } cpumask_t;
typedef struct cpumask *cpumask_var_t;

/**
 * cpumask_bits - get the bits in a cpumask
 * @maskp: the struct cpumask *
 *
 * You should assume nr_cpu_ids bits of this mask are valid. This is
 * a macro so it's const-correct.
 */
#define cpumask_bits(maskp)		((maskp)->bits)

#define nr_cpu_ids						NR_CPUS
#define nr_cpumask_bits				nr_cpu_ids

int alloc_cpumask_var(cpumask_var_t *mask);
void free_cpumask_var(cpumask_var_t mask);

/**
 * cpumask_first - get the first cpu in a cpumask
 * @srcp: the cpumask pointer
 *
 * Returns >= nr_cpu_ids if no cpus set.
 */
static inline unsigned int cpumask_first(const struct cpumask *srcp)
{
	return find_first_bit(cpumask_bits(srcp), nr_cpumask_bits);
}

/**
 * cpumask_next - get the next cpu in a cpumask
 * @n: the cpu prior to the place to search (ie. return will be > @n)
 * @srcp: the struct cpumask *
 *
 * Returns >= nr_cpu_ids if no further cpus set.
 */
static inline unsigned int cpumask_next(int n, const struct cpumask *srcp)
{
	return find_next_bit(cpumask_bits(srcp), nr_cpumask_bits, n + 1);
}

/**
 * cpumask_next_zero - get the next unset cpu in a cpumask
 * @n: the cpu prior to the place to search (ie. return will be > @n)
 * @srcp: the cpumask pointer
 *
 * Returns >= nr_cpu_ids if no further cpus unset.
 */
static inline unsigned int cpumask_next_zero(int n, const struct cpumask *srcp)
{
	return find_next_zero_bit(cpumask_bits(srcp), nr_cpumask_bits, n + 1);
}

int cpumask_next_and(int n, const struct cpumask *, const struct cpumask *);
int cpumask_any_but(const struct cpumask *mask, unsigned int cpu);

/**
 * for_each_cpu - iterate over every cpu in a mask
 * @cpu: the (optionally unsigned) integer iterator
 * @mask: the cpumask pointer
 *
 * After the loop, cpu is >= nr_cpu_ids.
 */
#define for_each_cpu(cpu, mask)										\
	for((cpu) = -1;																	\
		(cpu) = cpumask_next((cpu), (mask)),					\
		(cpu) < nr_cpu_ids;)	

/**
 * for_each_cpu_not - iterate over every cpu in a complemented mask
 * @cpu: the (optionally unsigned) integer iterator
 * @mask: the cpumask pointer
 *
 * After the loop, cpu is >= nr_cpu_ids.
 */
#define for_each_cpu_not(cpu, mask)								\
	for((cpu) = -1;																	\
		(cpu) = cpumask_next_zero((cpu), (mask)),			\
		(cpu) < nr_cpu_ids;)

/**
 * for_each_cpu_and - iterate over every cpu in both masks
 * @cpu: the (optionally unsigned) integer iterator
 * @mask: the first cpumask pointer
 * @and: the second cpumask pointer
 *
 * This saves a temporary CPU mask in many places.  It is equivalent to:
 *      struct cpumask tmp;
 *      cpumask_and(&tmp, &mask, &and);
 *      for_each_cpu(cpu, &tmp)
 *              ...
 *
 * After the loop, cpu is >= nr_cpu_ids.
 */
#define for_each_cpu_and(cpu, mask, and)							\
	for((cpu = -1);																			\
		(cpu) = cpumask_next_and((cpu), (mask), (and)),		\
		(cpu) < nr_cpu_ids;)

/**
 * cpumask_set_cpu - set a cpu in a cpumask
 * @cpu: cpu number (< nr_cpu_ids)
 * @dstp: the cpumask pointer
 */
static inline void cpumask_set_cpu(unsigned int cpu, struct cpumask *dstp)
{
	set_bit(cpu, cpumask_bits(dstp));
}

/**
 * cpumask_clear_cpu - clear a cpu in a cpumask
 * @cpu: cpu number (< nr_cpu_ids)
 * @dstp: the cpumask pointer
 */
static inline void cpumask_clear_cpu(int cpu, struct cpumask *dstp)
{
	clear_bit(cpu, cpumask_bits(dstp));
}

/**
 * cpumask_test_cpu - test for a cpu in a cpumask
 * @cpu: cpu number (< nr_cpu_ids)
 * @cpumask: the cpumask pointer
 */
static inline int cpumask_test_cpu(int cpu, const struct cpumask *dstp)
{
	return test_bit(cpu, cpumask_bits(dstp));
}

/**
 * cpumask_setall - set all cpus (< nr_cpu_ids) in a cpumask
 * @dstp: the cpumask pointer
 */
static inline void cpumask_setall(struct cpumask *dstp)
{
	bitmap_fill(cpumask_bits(dstp), nr_cpumask_bits);
}
 
/**
 * cpumask_clear - clear all cpus (< nr_cpu_ids) in a cpumask
 * @dstp: the cpumask pointer
 */
static inline void cpumask_clear(struct cpumask *dstp)
{
	bitmap_zero(cpumask_bits(dstp), nr_cpumask_bits);
}

/**
 * cpumask_and - *dstp = *src1p & *src2p
 * @dstp: the cpumask result
 * @src1p: the first input
 * @src2p: the second input
 */
static inline int cpumask_and(struct cpumask *dstp,
			       const struct cpumask *src1p,
			       const struct cpumask *src2p)
{
	return bitmap_and(cpumask_bits(dstp), cpumask_bits(src1p),
				       cpumask_bits(src2p), nr_cpumask_bits);
}

/**
 * cpumask_any - pick a "random" cpu from *srcp
 * @srcp: the input cpumask
 *
 * Returns >= nr_cpu_ids if no cpus set.
 */
#define cpumask_any(srcp) cpumask_first(srcp)

/**
 * cpumask_first_and - return the first cpu from *srcp1 & *srcp2
 * @src1p: the first input
 * @src2p: the second input
 *
 * Returns >= nr_cpu_ids if no cpus set in both.  See also cpumask_next_and().
 */
#define cpumask_first_and(src1p, src2p) cpumask_next_and(-1, (src1p), (src2p))

/*
 * cpumask_any_and - pick a "random" cpu from *mask1 & *mask2
 * @mask1: the first input cpumask
 * @mask2: the second input cpumask
 *
 * Returns >= nr_cpu_ids if no cpus set.
 */
#define cpumask_any_and(mask1, mask2) cpumask_first_and((mask1), (mask2))

#endif /* SCHED_RT */

#endif	/* __CPUMASK_H */
