/* Copyright (C) 1991,1992,1996,1997,1999,2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Douglas C. Schmidt (schmidt@ics.uci.edu).

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/* If you consider tuning this algorithm, you should consult first:
   Engineering a sort function; Jon Bentley and M. Douglas McIlroy;
   Software - Practice and Experience; Vol. 23 (11), 1249-1265, 1993.  */

#ifndef _QSORT_H_
#define _QSORT_H_

#include <alloca.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "compiler.h"

/* GNU's quicksort implementation. We have to compile with -D_GNU_SOURCE
 * and include their qsort.c in the project for this to link */
extern void _quicksort (void *const pbase, size_t total_elems, size_t size,
		__compar_d_fn_t cmp, void *arg);

struct qsort_def {
	size_t size;
	size_t align;
	int (*cmp)(const void *a, const void *b, void *context);
	void (*copy)(const struct qsort_def *def, void *dest, const void *src);
	void (*swap)(const struct qsort_def *def, void *a, void *b);
	//void (*swap)(const struct qsort_def *def, void *a, void *b, void *tmp, size_t tmp_size);
	void (*ror) (const struct qsort_def *def, void *left, void *right, void *tmp, size_t tmp_size);
};

static __always_inline void
_quicksort_copy(const struct qsort_def *def, void *dest, const void *src) {
	BUILD_BUG_ON_MSG(1, "not implemented");
}

static __always_inline void
_quicksort_swap(const struct qsort_def *def, void *a, void *b) {
	BUILD_BUG_ON_MSG(1, "not implemented");
}

/*
 * _quicksort_ror -- move data element left, shifting all elements in between
 * 		     to the right
 * left:	leftmost element
 * right:	rightmost element
 * tmp:		a temporary buffer to use
 * tmp_size:	size of tmp
 */
static __always_inline void
_quicksort_ror(const struct qsort_def *def, void *left, void *right, void *tmp, size_t tmp_size) {
	char *r = right;
	char *l = left;
	const ssize_t dist = (l - r) / (ssize_t)def->size; /* right to left offset */

	void (* const copy)(const struct qsort_def *def, void *dest, const void *src)
		= def->copy ? def->copy : _quicksort_copy;
	void (* const swap)(const struct qsort_def *def, void *a, void *b)
		= def->swap ? def->swap : _quicksort_swap;

	/* validate pointer alignment */
	assert_early(!((uintptr_t)l & (def->align - 1)));
	assert_early(!((uintptr_t)r & (def->align - 1)));

	/* validate distance between pointers is correct multiple */
	assert_early(!((r - l) & (def->size - 1)));
	assert_early(dist != 0);
	//assert_early(dist < (ssize_t)0);	/* should be negative */
	//assert_const(dist < 0);
	if (__builtin_constant_p(dist)){}

	if (dist == -1)
		swap(def, left, right);
	else if (def->size <= tmp_size) {
		ssize_t i;		/* here, i is a negative elem. index */
		char *right_plus_one = r + def->size;

		copy(def, tmp, r);

		/* rep movs-friendly loop */
		for (i = dist + 1; i; ++i)
			copy(def, &right_plus_one[i], &r[i]);

		copy(def, left, tmp);
	} else {
		//BUILD_BUG_ON_MSG(1, "large element sizes not yet implemented");

		/* tmp_size < size, so we have to copy elements in chunks */
		ssize_t i;		/* here, i is a negative byte index */
		//ssize_t off;		/* chunk offset */
		char *right_plus_one = r + def->size;

		/* This algo iteratively moves the leftmost element right and
		 * is designed to not thrash the cache when the range between
		 * left and right is large.
		 *
		 * Caveats:
		 * - It is less efficient for smaller ranges
		 * - It will perform poorly if either the element size is
		 *   larger than the data cache or the CPU has no cache. */
		for (i = (dist + 1) * def->size; i; i += def->size) {

			/* FIXME: large tmp buffer wasted here */
			swap(def, &r[i], &right_plus_one[i]);

#if 0
			size_t chunk_size = ((def->size - 1) % tmp_size) + 1;

			for (off = def->size - chunk_size; off < def->size; off -= tmp_size) {
				char *d = &r[i + off];
				char *s = &right_plus_one[i + off];

				memcpy(tmp, d,   chunk_size);
				memcpy(d,   s,   chunk_size);
				memcpy(s,   tmp, chunk_size);

				chunk_size = tmp_size;
			}
#endif
		}
	}
}

/* Discontinue quicksort algorithm when partition gets below this size.
   This particular magic number was chosen to work best on a Sun 4/260. */
#define MAX_THRESH 4

/* Stack node declarations used to store unfulfilled partition obligations. */
typedef struct
  {
    char *lo;
    char *hi;
  } stack_node;

/* The next 4 #defines implement a very fast in-line stack abstraction. */
/* The stack needs log (total_elements) entries (we could even subtract
   log(MAX_THRESH)).  Since total_elements has type size_t, we get as
   upper bound for log (total_elements):
   bits per byte (CHAR_BIT) * sizeof(size_t).  */
#define STACK_SIZE	(CHAR_BIT * sizeof(size_t))
#define PUSH(low, high)	((void) ((top->lo = (low)), (top->hi = (high)), ++top))
#define	POP(low, high)	((void) (--top, (low = top->lo), (high = top->hi)))
#define	STACK_NOT_EMPTY	(stack < top)


/* Order size using quicksort.  This implementation incorporates
   four optimizations discussed in Sedgewick:

   1. Non-recursive, using an explicit stack of pointer that store the
      next array partition to sort.  To save time, this maximum amount
      of space required to store an array of SIZE_MAX is allocated on the
      stack.  Assuming a 32-bit (64 bit) integer for size_t, this needs
      only 32 * sizeof(stack_node) == 256 bytes (for 64 bit: 1024 bytes).
      Pretty cheap, actually.

   2. Chose the pivot element using a median-of-three decision tree.
      This reduces the probability of selecting a bad pivot value and
      eliminates certain extraneous comparisons.

   3. Only quicksorts TOTAL_ELEMS / MAX_THRESH partitions, leaving
      insertion sort to order the MAX_THRESH items within each partition.
      This is a big win, since insertion sort is faster for small, mostly
      sorted array segments.

   4. The larger of the two sub-partitions is always pushed onto the
      stack first, with the algorithm then concentrating on the
      smaller partition.  This *guarantees* no more than log (total_elems)
      stack size is needed (actually O(1) in this case)!  */

static __always_inline void
_quicksort_template (const struct qsort_def *def, void *const pbase, size_t total_elems, void *arg)
{
  register char *base_ptr = (char *) pbase;
  const size_t max_thresh = MAX_THRESH * def->size;
  const size_t size       = def->size;
  const size_t align      = def->align;

  void (* const swap)(const struct qsort_def *def, void *a, void *b)
        = def->swap ? def->swap : _quicksort_swap;
  void (* const ror) (const struct qsort_def *def, void *left, void *right, void *tmp, size_t tmp_size)
        = def->ror ? def->ror : _quicksort_ror;

  assert_const(def->align + def->size);
  BUILD_BUG_ON_MSG(!def->cmp, "cmp function is required");

  /* align must be a power of two */
  assert_early(!(def->align &(def->align - 1)));

  /* size must be a multiple of align */
  assert_early(!(def->align % def->size));

  /* verify pbase is really aligned as advertised */
  assert_early(!((uintptr_t)pbase & (def->align - 1)));

  if (total_elems == 0)
    /* Avoid lossage with unsigned arithmetic below.  */
    return;

  if (total_elems > MAX_THRESH)
    {
      char *lo = base_ptr;
      char *hi = &lo[size * (total_elems - 1)];
      stack_node stack[STACK_SIZE];
      stack_node *top = stack;

      PUSH (NULL, NULL);

      while (STACK_NOT_EMPTY)
        {
          char *left_ptr;
          char *right_ptr;

          /* Select median value from among LO, MID, and HI. Rearrange
             LO and HI so the three values are sorted. This lowers the
             probability of picking a pathological pivot value and
             skips a comparison for both the LEFT_PTR and RIGHT_PTR in
             the while loops. */

          char *mid = lo + size * ((hi - lo) / size >> 1);

          if ((*def->cmp) ((void *) mid, (void *) lo, arg) < 0)
            swap (def, mid, lo);
          if ((*def->cmp) ((void *) hi, (void *) mid, arg) < 0)
            swap (def, mid, hi);
          else
            goto jump_over;
          if ((*def->cmp) ((void *) mid, (void *) lo, arg) < 0)
            swap (def, mid, lo);
        jump_over:;

          left_ptr  = lo + size;
          right_ptr = hi - size;

          /* Here's the famous ``collapse the walls'' section of quicksort.
             Gotta like those tight inner loops!  They are the main reason
             that this algorithm runs much faster than others. */
          do
            {
              while ((*def->cmp) ((void *) left_ptr, (void *) mid, arg) < 0)
                left_ptr += size;

              while ((*def->cmp) ((void *) mid, (void *) right_ptr, arg) < 0)
                right_ptr -= size;

              if (left_ptr < right_ptr)
                {
                  swap (def, left_ptr, right_ptr);
                  if (mid == left_ptr)
                    mid = right_ptr;
                  else if (mid == right_ptr)
                    mid = left_ptr;
                  left_ptr += size;
                  right_ptr -= size;
                }
              else if (left_ptr == right_ptr)
                {
                  left_ptr += size;
                  right_ptr -= size;
                  break;
                }
            }
          while (left_ptr <= right_ptr);

          /* Set up pointers for next iteration.  First determine whether
             left and right partitions are below the threshold size.  If so,
             ignore one or both.  Otherwise, push the larger partition's
             bounds on the stack and continue sorting the smaller one. */

          if ((size_t) (right_ptr - lo) <= max_thresh)
            {
              if ((size_t) (hi - left_ptr) <= max_thresh)
                /* Ignore both small partitions. */
                POP (lo, hi);
              else
                /* Ignore small left partition. */
                lo = left_ptr;
            }
          else if ((size_t) (hi - left_ptr) <= max_thresh)
            /* Ignore small right partition. */
            hi = right_ptr;
          else if ((right_ptr - lo) > (hi - left_ptr))
            {
              /* Push larger left partition indices. */
              PUSH (lo, right_ptr);
              lo = left_ptr;
            }
          else
            {
              /* Push larger right partition indices. */
              PUSH (left_ptr, hi);
              hi = right_ptr;
            }
        }
    }

  /* Once the BASE_PTR array is partially sorted by quicksort the rest
     is completely sorted using insertion sort, since this is efficient
     for partitions below MAX_THRESH size. BASE_PTR points to the beginning
     of the array to sort, and END_PTR points at the very last element in
     the array (*not* one beyond it!). */

#define min(x, y) ((x) < (y) ? (x) : (y))
  {
    /* actually adds up to 63 more to this */
    const size_t MAX_STACK_SIZE = sizeof(stack_node) * STACK_SIZE;
    int tmp_on_heap = 0;
    size_t tmp_size = 0;
    void *tmp;

    if (MAX_STACK_SIZE < size)
      {
        tmp_size = size;
        tmp = aligned_alloc(align, size);
        if (tmp)
          tmp_on_heap = 1;
      }

    if (!tmp_on_heap)
      {
        size_t offset;
        /* put it on the stack */
        tmp_size = min(size, MAX_STACK_SIZE);
        tmp = alloca (tmp_size + align - 1);

        /* align it */
        offset = (uintptr_t)tmp % align;
        if (offset)
          tmp += align - offset;
      }

    /* if element size is a power of two, indexed addressing will be more efficient in most cases */
    if (!(def->size & (def->size - 1)))
      {
        const size_t thresh = min (total_elems, MAX_THRESH + 1);
        size_t left, right;
        size_t smallest;

#ifdef ARCH_MAX_INDEX_MULT
        if (def->size > ARCH_MAX_INDEX_MULT)
          goto ptr_based;
#endif
        /* Find smallest element in first threshold and place it at the
           array's beginning.  This is the smallest array element,
           and the operation speeds up insertion sort's inner loop. */

        for (smallest = 0, right = 1; right < thresh; ++right)
          if (def->cmp (&base_ptr[right * def->size], base_ptr, arg) < 0)
            smallest = right;
        if (smallest)
          swap (def, &base_ptr[smallest * def->size], base_ptr);

        for (right = 1; right < total_elems; ++right)
          {
            void *pr, *pl;
            left = right;
            do
              {
                --left;
                pr = &base_ptr[right * def->size];
                pl = &base_ptr[left  * def->size];
                if (def->cmp (pr, pl, arg) >= 0)
                  break;
              }
            while (left);

            if (left)
              _quicksort_ror(def, pl, pr, tmp, tmp_size);
          }
      }
    else
      {

        /* if not a power of two, use ptr arithmetic */
        const size_t size = def->size;
        char *const end_ptr = &base_ptr[size * (total_elems - 1)];
        char *tmp_ptr = base_ptr;
        char *thresh = min(end_ptr, base_ptr + max_thresh);
        register char *run_ptr;

        /* Find smallest element in first threshold and place it at the
           array's beginning.  This is the smallest array element,
           and the operation speeds up insertion sort's inner loop. */

        for (run_ptr = tmp_ptr + size; run_ptr <= thresh; run_ptr += size)
          if ((*def->cmp) ((void *) run_ptr, (void *) tmp_ptr, arg) < 0)
            tmp_ptr = run_ptr;

        if (tmp_ptr != base_ptr)
          swap (def, tmp_ptr, base_ptr);

        /* Insertion sort, running from left-hand-side up to right-hand-side.  */

        run_ptr = base_ptr + size;
        while ((run_ptr += size) <= end_ptr)
          {
            tmp_ptr = run_ptr - size;
            while ((*def->cmp) ((void *) run_ptr, (void *) tmp_ptr, arg) < 0)
              tmp_ptr -= size;

            tmp_ptr += size;
            if (tmp_ptr != run_ptr)
              ror(def, tmp_ptr, run_ptr, tmp, tmp_size);
          }
      }
  }
}

#endif /* _QSORT_H_ */
