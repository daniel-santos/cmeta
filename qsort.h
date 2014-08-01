/* Copyright (C) 1991,1992,1996,1997,1999,2004 Free Software Foundation, Inc.
 * Copyright (C) 2014 Daniel Santos <daniel.santos@pobox.com>

   This is a modified version of stdlib/qsort.c from the the GNU C Library
   version 2.17 originally written by Douglas C. Schmidt (schmidt@ics.uci.edu).
   It has been modified to implement the qsort algorithm as a C pseudo-template
   function. Note that this is not what you get when you call qsort or qsort_r
   with glibc, as it will prefer to use an msort function but use
   _quicksort when it cannot allocate enough memory or some such.


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

#define _GNU_SOURCE
#include <alloca.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdalign.h>

#include "compiler.h"
/* GNU's quicksort implementation. We have to define _GNU_SOURCE prior to
 * including stddef.h and include their qsort.c in the project for this to
 * link */
extern void _quicksort (void *const pbase, size_t total_elems, size_t size,
		__compar_d_fn_t cmp, void *arg);


/* struct qsort_def -- pseudo-template definition for _quicksort_template */
struct qsort_def {
	size_t size;
	size_t align;
	//int (*cmp)(const void *a, const void *b, void *context);
	int (*less)(const void *a, const void *b, void *context);
	//int (*equal)(const void *a, const void *b, void *context);

	//void (*copy)(const struct qsort_def *def, void *dest, const void *src);
	//void (*swap)(const struct qsort_def *def, void *a, void *b);
	//void (*swap)(const struct qsort_def *def, void *a, void *b, void *tmp, size_t tmp_size);
	//void (*ror) (const struct qsort_def *def, void *left, void *right, void *tmp, size_t tmp_size);
};

#if 0
/* attempt to write a good memcpy in C.... uhh, maybe later */
static __always_inline void
_quicksort_copy(const struct qsort_def *def, void *dest, const void *src) {
	const size_t size = def->size;
	const size_t align = def->align;
	size_t fast_size;
	size_t off = 0;

	/* for the given size, figure out what the optimal number of bytes to copy
	 * aligned would be on this platform */
	if (size == 1) {
		fast_size = sizeof(__UINT_FAST8_TYPE__);
	} else if (size < 4) {
		fast_size = sizeof(__UINT_FAST16_TYPE__);
	} else if (size < 8) {
		fast_size = sizeof(__UINT_FAST32_TYPE__);
	} else if (size < _Alignof(max_align_t)) {
#ifdef __UINT_LEAST64_TYPE__
		fast_size = sizeof(__UINT_FAST64_TYPE__);
#else
		fast_size = sizeof(__UINT_FAST32_TYPE__);
#endif
	} else {
		fast_size = _Alignof(max_align_t);
	}

	/* if alignment is smaller than fast_size, we'll slow copy until aligned to
	 * fast_size */
	if (align < fast_size) {
		/* copy and then set off to where to start aligned copy */
	}

	/* do fast copy */

	/* if there's any unaligned at the end, copy that */
}
#endif

#define _ALIGNED_COPY(def, dest, src, align)				\
	do {								\
		void       *d = __builtin_assume_aligned(dest, align);	\
		const void *s = __builtin_assume_aligned(src, align);	\
		__builtin_memcpy(d, s, def->size);			\
		return;                                 		\
	} while (0)

#define min(x, y) ((x) < (y) ? (x) : (y))

static __always_inline void
_quicksort_copy(const struct qsort_def *def, void *dest, const void *src) {
	/* control the size of align */
	const size_t align = min(def->align, _Alignof(max_align_t));

        assert_const(align);
//BUILD_BUG_ON_MSG(1, "not implemented");
//assert(0);
	switch (align) {
	case 1:   _ALIGNED_COPY(def, dest, src, 1);
	case 2:   _ALIGNED_COPY(def, dest, src, 2);
	case 4:   _ALIGNED_COPY(def, dest, src, 4);
	case 8:   _ALIGNED_COPY(def, dest, src, 8);
	case 16:  _ALIGNED_COPY(def, dest, src, 16);
	case 32:  _ALIGNED_COPY(def, dest, src, 32);
	case 64:  _ALIGNED_COPY(def, dest, src, 64);
	case 128: _ALIGNED_COPY(def, dest, src, 128);
	case 256: _ALIGNED_COPY(def, dest, src, 256);
	default:  BUILD_BUG();
	}
}

static __always_inline void
_quicksort_swap(const struct qsort_def *def, void *a, void *b, void *tmp, size_t tmp_size) {
	_quicksort_copy(def, tmp, a);
	_quicksort_copy(def, a, b);
	_quicksort_copy(def, b, tmp);
//BUILD_BUG_ON_MSG(1, "not implemented");
//assert(0);
}

/*
 * _quicksort_ror -- move data element at right to left, shifting all elements
 * 		     in between to the right
 * def:         the template parameters
 * left:	leftmost element
 * right:	rightmost element
 * tmp:		a temporary buffer to use
 * tmp_size:	size of tmp
 */
static __always_inline __flatten void
_quicksort_ror(const struct qsort_def *def, void *left, void *right, void *tmp, size_t tmp_size) {
        const ssize_t size = (ssize_t)def->size;
	char *r = right;
	char *l = left;
	const ssize_t dist = (r - l) / (ssize_t)def->size; /* left to right offset */

#if 0
	assert_const(!def->copy);
	assert_const(!def->swap);
	BUILD_BUG_ON(!def->copy);
	BUILD_BUG_ON(!def->swap);


	void (* const copy)(const struct qsort_def *def, void *dest, const void *src)
		=  !!(def->copy) ? def->copy : _quicksort_copy;
	void (* const swap)(const struct qsort_def *def, void *a, void *b, void *tmp, size_t tmp_size)
		=  !!(def->swap) ? def->swap : _quicksort_swap;
#endif
	//left = __builtin_assume_aligned(left, def->align);
	//left = __builtin_assume_aligned(left, ((uintptr_t)left >> 5) & 15);


	/* validate pointer alignment */
	assert_early(!((uintptr_t)l & (def->align - 1)));
	assert_early(!((uintptr_t)r & (def->align - 1)));

	/* validate distance between pointers is positive */
        assert(dist != 0);
        assert(dist > 0);

	if (size <= (ssize_t)tmp_size) {
		ssize_t i;
		char *left_minus_one = l - size;

		_quicksort_copy(def, tmp, r);
		/* rep movs-friendly loop */
		for (i = dist; i; --i)
			_quicksort_copy(def, &l[i * size], &left_minus_one[i * size]);

		_quicksort_copy(def, left, tmp);
	} else {

		/* tmp_size < size, so we have to copy elements in chunks */
		ssize_t i;		/* here, i is a negative byte index */
		//ssize_t off;		/* chunk offset */
		char *right_plus_one = r + size;

		//BUILD_BUG_ON_MSG(1, "large element sizes not yet implemented");

		/* This algo iteratively moves the leftmost element right and
		 * is designed to not thrash the cache when the range between
		 * left and right is large.
		 *
		 * Caveats:
		 * - It is less efficient for smaller ranges
		 * - It will perform poorly if either the element size is
		 *   larger than the data cache or the CPU has no cache. */

		for (i = -(dist - 1) * size; i <= 0; i += size)
			_quicksort_swap(def, &r[i * size], &right_plus_one[i * size], tmp, tmp_size);

#if 0
		for (i = -(dist - 1) * size; i <= 0; i += size) {
			size_t chunk_size = ((size - 1) % tmp_size) + 1;

			for (off = size - chunk_size; off < size; off -= tmp_size) {
				char *d = &r[i + off];
				char *s = &right_plus_one[i + off];

				memcpy(tmp, d,   chunk_size);
				memcpy(d,   s,   chunk_size);
				memcpy(s,   tmp, chunk_size);

				chunk_size = tmp_size;
			}
		}
#endif
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

static __always_inline __flatten void
_quicksort_template (const struct qsort_def *def, void *const pbase, size_t total_elems, void *arg)
{
  register char *base_ptr = (char *) pbase;
  const size_t max_thresh = MAX_THRESH * def->size;
  const size_t size       = def->size;

#if __STDC_VERSION__ >= 201112L
                            /* restrict to meaningful value */
  const size_t align      = min (def->align, _Alignof(max_align_t));
#else
  const size_t align      = min (def->align, 256);
#endif

#if 0
  void (* const swap)(const struct qsort_def *def, void *a, void *b, void *tmp, size_t tmp_size)
        = !!(def->swap) ? def->swap : _quicksort_swap;
  void (* const ror) (const struct qsort_def *def, void *left, void *right, void *tmp, size_t tmp_size)
        = /* def->ror ? def->ror : */ _quicksort_ror;
#endif
  /* actually adds up to (align - q) more to this */
  const size_t MAX_STACK_SIZE = 256;
  int tmp_on_heap = 0;
  size_t tmp_size = 0;
  void *tmp;

  assert_const(!!(def->less));

#if 0
  assert(def->copy);
  assert(swap);
  assert(ror);
#endif

  assert_const(def->align + def->size);
  BUILD_BUG_ON_MSG(!def->less, "less function is required");
#if __STDC_VERSION__ >= 201112L
  BUILD_BUG_ON_MSG(_Alignof(max_align_t) & (_Alignof(max_align_t) - 1),
                   "_Alignof(max_align_t) is not a power of two");
#endif
  BUILD_BUG_ON_MSG(align & (align - 1), "align must be a power of two");

  /* size must be a multiple of align */
  BUILD_BUG_ON(def->size % def->align);

  /* verify pbase is really aligned as advertised */
  assert_early(!((uintptr_t)pbase & (def->align - 1)));

  if (total_elems == 0)
    /* Avoid lossage with unsigned arithmetic below.  */
    return;

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

          if ((*def->less) ((void *) mid, (void *) lo, arg))
            _quicksort_swap (def, mid, lo, tmp, tmp_size);
          if ((*def->less) ((void *) hi, (void *) mid, arg))
            _quicksort_swap (def, mid, hi, tmp, tmp_size);
          else
            goto jump_over;
          if ((*def->less) ((void *) mid, (void *) lo, arg))
            _quicksort_swap (def, mid, lo, tmp, tmp_size);
        jump_over:;

          left_ptr  = lo + size;
          right_ptr = hi - size;

          /* Here's the famous ``collapse the walls'' section of quicksort.
             Gotta like those tight inner loops!  They are the main reason
             that this algorithm runs much faster than others. */
          do
            {
              while ((*def->less) ((void *) left_ptr, (void *) mid, arg))
                left_ptr += size;

              while ((*def->less) ((void *) mid, (void *) right_ptr, arg))
                right_ptr -= size;

              if (left_ptr < right_ptr)
                {
                  _quicksort_swap (def, left_ptr, right_ptr, tmp, tmp_size);
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


  /* if element size is a power of two, indexed addressing will be more efficient in most cases */
  if (!(def->size & (def->size - 1)))
    {
      const size_t thresh = min (total_elems, MAX_THRESH + 1);
      size_t left, right;
      void *smallest;
//      BUILD_BUG();
#ifdef ARCH_MAX_INDEX_MULT
      if (def->size > ARCH_MAX_INDEX_MULT)
        goto ptr_based;
#endif
      /* Find smallest element in first threshold and place it at the
          array's beginning.  This is the smallest array element,
          and the operation speeds up insertion sort's inner loop. */

      for (smallest = base_ptr, right = 1; right < thresh; ++right)
        if (def->less (&base_ptr[right * def->size], smallest, arg))
          smallest = &base_ptr[right * def->size];
      if (smallest != base_ptr)
        _quicksort_swap (def, smallest, base_ptr, tmp, tmp_size);
//__asm("int $3");
      for (right = 2; right < total_elems; ++right)
        {
          left = right - 1;
          while (def->less (&base_ptr[right * def->size], &base_ptr[left * def->size], arg))
            --left;

          ++left;
          if (left != right)
            _quicksort_ror(def, &base_ptr[left * def->size], &base_ptr[right * def->size], tmp, tmp_size);
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
        if ((*def->less) ((void *) run_ptr, (void *) tmp_ptr, arg))
          tmp_ptr = run_ptr;

      if (tmp_ptr != base_ptr)
        _quicksort_swap (def, tmp_ptr, base_ptr, tmp, tmp_size);

      /* Insertion sort, running from left-hand-side up to right-hand-side.  */

      run_ptr = base_ptr + size;
      while ((run_ptr += size) <= end_ptr)
        {
          tmp_ptr = run_ptr - size;
          while ((*def->less) ((void *) run_ptr, (void *) tmp_ptr, arg))
            tmp_ptr -= size;

          tmp_ptr += size;
          if (tmp_ptr != run_ptr)
              _quicksort_ror(def, tmp_ptr, run_ptr, tmp, tmp_size);
        }
    }

  if (tmp_on_heap)
	  free (tmp);
}

#endif /* _QSORT_H_ */
