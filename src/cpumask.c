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

#include "cpumask.h"

#ifdef SCHED_RT

/* from /lib/bitmap.c */
int __bitmap_and(unsigned long *dst, const unsigned long *bitmap1,
								const unsigned long *bitmap2, int bits)
{
	int k;
	int nr = BITS_TO_LONGS(bits);
	unsigned long result = 0;

	for (k = 0; k < nr; k++)
		result |= (dst[k] = bitmap1[k] & bitmap2[k]);
	return result != 0;
}

/* from /lib/find_next_bit.c */

#define BITOP_WORD(nr)					((nr) / BITS_PER_LONG)

/**
 * Find the first set bit in a memory region
 */
unsigned long find_first_bit(const unsigned long *addr, unsigned long size)
{
	const unsigned long *p = addr;
	unsigned long result = 0;
	unsigned long tmp;

	/**
	 * addr is an array of long, so
	 * we have to search for the first
	 * long that has a set bit in the
	 * array, starting from index 0.
	 * If no such long is found, then
	 * we mustn't call __ffs()
	 */
	while(size & ~(BITS_PER_LONG - 1)) {
		if((tmp = *(p++)))
			goto found;
		result += BITS_PER_LONG;
		size -= BITS_PER_LONG;
	}
	/**
	 * if we are here size is a multiple of 
	 * BITS_PER_LONG and there are no set bits
	 * in any variable of the mask
	 */ 
	if(!size)
		return result;

	/**
	 * size is not a multiple of BITS_PER_LONG
	 * so we have to check the last (BITS_PER_LONG -
	 * size) bits
	 */
	tmp = (*p) & (~0UL >> (BITS_PER_LONG - size));
	if(tmp == 0UL)				/* Are any bits set? */
		return result + size;		/* Nope */

found:
	return result + __ffs(tmp);
}

/**
 * Find the next set bit in a memory region.
 */
unsigned long find_next_bit(const unsigned long *addr, unsigned long size,
														unsigned long offset)
{
	const unsigned long *p = addr + BITOP_WORD(offset);
	unsigned long result = offset & ~(BITS_PER_LONG - 1);
	unsigned long tmp;

	if(offset >= size)
		return size;

	/**
	 * if offset % BITS_PER_LONG != 0
	 * we have to check the remaining bits
	 * of *p first 
	 */
	size -= result;
	offset %= BITS_PER_LONG;
	if(offset) {
		tmp = *(p++);
		tmp &= (~0UL << offset);
		if(size < BITS_PER_LONG)
			goto found_first;
		if(tmp)
			goto found_middle;
		size -= BITS_PER_LONG;
		result += BITS_PER_LONG;	
	}
	/* same as find_first_bit() */
	while(size & ~(BITS_PER_LONG - 1)) {
		if((tmp = *(p++)))
			goto found_middle;
		result += BITS_PER_LONG;
		size -= BITS_PER_LONG;	
	}
	if(!size)
		return result;
	tmp = *p;

found_first:
	tmp &= (~0UL >> (BITS_PER_LONG - size));
	if(tmp == 0UL)			/* Are there any bits set? */
		return result + size;
found_middle:
	return result + __ffs(tmp);
}

unsigned long find_next_zero_bit(const unsigned long *addr, unsigned long size,
                                 unsigned long offset)
{
	const unsigned long *p = addr + BITOP_WORD(offset);
	unsigned long result = offset & ~(BITS_PER_LONG-1);
	unsigned long tmp;

	/**
	 * same as find_next_bit but since
	 * we're searching for unset bits,
	 * we have to use | operator to mask
	 * long values
	 */
	if (offset >= size)
		return size;
	size -= result;
	offset %= BITS_PER_LONG;
	if (offset){
		tmp = *(p++);
		tmp |= ~0UL >> (BITS_PER_LONG - offset);
		if (size < BITS_PER_LONG)
			goto found_first;
		if (~tmp)
			goto found_middle;
		size -= BITS_PER_LONG;
		result += BITS_PER_LONG;
	}
	while (size & ~(BITS_PER_LONG-1)) {
		if (~(tmp = *(p++)))
			goto found_middle;
		result += BITS_PER_LONG;
		size -= BITS_PER_LONG;
	}
	if (!size)
		return result;
	tmp = *p;

found_first:
	tmp |= ~0UL << size;
	if (tmp == ~0UL)        /* Are any bits zero? */
		return result + size;   /* Nope. */
found_middle:
		/** 
		 * for portability purpose we implement ffz()
		 * in term of __ffs()
		 */
		return result + __ffs(~tmp);
}

/**
 * alloc_cpumask_var - allocate a struct cpumask
 * @mask:	cpumask_var_t pointer
 */
int alloc_cpumask_var(cpumask_var_t *mask)
{
	*mask = (cpumask_var_t)calloc(1, sizeof(cpumask_t));
	if(!(*mask))
		return 0;
	else
		return 1;
}

/**
 * free_cpumask_var - free a struct cpumask
 * @mask:	cpumask_var_t variable
 */
void free_cpumask_var(cpumask_var_t mask)
{
	free(mask);
}

/**
 * cpumask_next_and - get the next cpu in *src1p & *src2p
 * @n: the cpu prior to the place to search (ie. return will be > @n)
 * @src1p: the first cpumask pointer
 * @src2p: the second cpumask pointer
 *
 * Returns >= nr_cpu_ids if no further cpus set in both.
 */
int cpumask_next_and(int n, const struct cpumask *src1p, 
										const struct cpumask *src2p)
{
	while((n = cpumask_next(n, src1p)) < nr_cpu_ids)
		if(cpumask_test_cpu(n, src2p))
			break;

	return n;
}

/**
 * cpumask_any_but - return a "random" in a cpumask, but not this one.
 * @mask: the cpumask to search
 * @cpu: the cpu to ignore.
 *
 * Returns >= nr_cpu_ids if no cpus set.
 */
int cpumask_any_but(const struct cpumask *mask, unsigned int cpu)
{
	unsigned int i;

	for_each_cpu(i, mask)
		if(i != cpu)
			break;

	return i;
}

#endif /* SCHED_RT */
