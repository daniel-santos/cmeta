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
#include <stddef.h>
#include <alloca.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#if __STDC_VERSION__ >= 201112L
# include <stdalign.h>
# define _QSORT_ALIGN_MAX _Alignof(max_align_t)
#else
# define _QSORT_ALIGN_MAX 128
#endif

#include "compiler.h"

/* GNU's quicksort implementation. We have to define _GNU_SOURCE prior to
 * including stddef.h and include their qsort.c in the project for this to
 * link */
extern void _quicksort (void *const pbase, size_t total_elems, size_t size,
		__compar_d_fn_t cmp, void *arg);

/* Do a little CPU sniffing to try determine most efficient algo. ARM uses LSR
 * extension to cover any index that's a power of two, but intel only has 1, 2,
 * 4, and 8.
 * FIXME: Only ARM & x86 are considered here. */
#if defined(__i386) || defined (__i386__)
# define _QSORT_ARCH_MAX_INDEX_MULT 8
#endif

#ifndef _QSORT_ARCH_MAX_INDEX_MULT
# define _QSORT_ARCH_MAX_INDEX_MULT __SIZE_MAX__
#endif

/* Threshold of element size before switching to an indirect sort. */
#ifndef _QSORT_IND_THRESH
# define _QSORT_IND_THRESH 64
#endif

/* struct qsort_def -- pseudo-template definition for _quicksort_template */


/**
 * struct qsort_def --
 *
 */
/**
 * @struct qsort_def
 * @brief pseudo-template definition and params for qsort_template
 * @var qsort_def::size
 * Element size
 *
 * @var qsort_def::align
 * Minimum alignment of elements
 *
 * @var qsort_def::less
 *
 * @var qsort_def::elem_buf
 * Pointer to an element buffer (should be at least size bytes and alisnged).
 *
 * @var qsort_def::index
 * Pointer to an index buffer when indirect sorting is used.
 *
 * @var qsort_def::max_count_bits
 * Maximum number of bits needed to store count of elements. e.g., if
 * max_count_bits is 16, then the qsort will only allocate enough stack and/or
 * heap space to sort arrays with 2^16 - 1 (65535) elements in them.
 *
 * @var qsort_def::max_stack_space
 * The maxiumum number of bytes to put on the stack. If needed space bypasses
 * this value, memory is allocated on the heap.
 *
 * @var qsort_def::max_thresh
 *
 */
struct qsort_def {
	size_t size;
	size_t align;
	int (*less)(const void *a, const void *b, void *context);
	void *elem_buf;
	void **index;
	size_t max_size_bits;
	size_t max_stack_space;
	size_t max_thresh;
};

#define _ALIGNED_COPY(def, dest, src, align)                            \
	do {                                                            \
		void       *d = __builtin_assume_aligned(dest, align);  \
		const void *s = __builtin_assume_aligned(src, align);   \
		__builtin_memcpy(d, s, def->size);                      \
		return;                                                 \
	} while (0)

#define min(x, y) ((x) < (y) ? (x) : (y))

static __always_inline void
_quicksort_copy(const struct qsort_def *def, void *dest, const void *src) {
	/* control the size of align */
	const size_t align = min(def->align, _Alignof(max_align_t));

	assert_const(align);

	/* A switch statement is used here to bring the value align from a
	 * compile-time constant back into a literal constant usable by
	 * __builtin_assume_aligned() and macro pasting. Through dead code
	 * removal, the optimizer will remove the all code (the switch
	 * statement, cases and bodies of non-matching cases) required for this
	 * mutation. */
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

#undef _ALIGNED_COPY


static __always_inline void
_quicksort_swap(const struct qsort_def *def, void *a, void *b) {
	_quicksort_copy(def, def->elem_buf, a);
	_quicksort_copy(def, a, b);
	_quicksort_copy(def, b, def->elem_buf);
}

/*
 * _quicksort_ror -- move data element at right to left, shifting all elements
 *                   in between to the right
 * def:         the template parameters
 * left:        leftmost element
 * right:       rightmost element
 */
static __always_inline __flatten void
_quicksort_ror(const struct qsort_def *def, void *left, void *right) {
	const ssize_t size = (ssize_t)def->size;
	char *r = right;
	char *l = left;
	char *left_minus_one = l - size;
	const ssize_t dist = (r - l) / (ssize_t)def->size; /* left to right offset */
	ssize_t i;

	/* validate pointer alignment */
	assert_early(!((uintptr_t)l & (def->align - 1)));
	assert_early(!((uintptr_t)r & (def->align - 1)));

	/* validate distance between pointers is positive */
	assert(dist != 0);
	assert(dist > 0);

	_quicksort_copy(def, def->elem_buf, r);

	/* x86 rep movs-friendly loop */
	for (i = dist; i; --i)
		_quicksort_copy(def, &l[i * size], &left_minus_one[i * size]);

	_quicksort_copy(def, left, def->elem_buf);
}

static __always_inline __flatten int
_quicksort_less(const struct qsort_def *def, int indirect, void *a, void *b, void *arg) {
  return indirect
       ? def->less(*((void**)a), *((void**)b), arg)
       : def->less(a, b, arg);
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
   bits per byte (CHAR_BIT) * sizeof(size_t).
   However, this results in 1k of stack being allocated on 64 bit machines,
   which is more than currently reasonable. Thus, for such machines, we'll cap
   the max entries to 48 bits, or 281 trillion.
   TODO: Add some mechanism to override this cap?
   */
#if (CHAR_BIT * __SIZEOF_SIZE_T__) < 48
# define STACK_SIZE     (CHAR_BIT * sizeof(size_t))
#else
# define STACK_SIZE     48
#endif

#define PUSH(low, high)	((void) ((top->lo = (low)), (top->hi = (high)), ++top))
#define	POP(low, high)	((void) (--top, (low = top->lo), (high = top->hi)))
#define	STACK_NOT_EMPTY	(stack < top)


/* Order size using quicksort.  This implementation incorporates
   four optimizations discussed in Sedgewick:

   1. Non-recursive, using an explicit stack of pointer that store the
      next array partition to sort.  To save time, this maximum amount
      of space required to store an array of SIZE_MAX is allocated on the
      stack.  Assuming a 32-bit (64 bit) integer for size_t, this needs
      only 32 * sizeof(stack_node) == 256 bytes (for 64 bit: 768 bytes,
      or 1024 bytes if uncapped). Pretty cheap, actually.

   2. Chose the pivot element using a median-of-three decision tree.
      This reduces the probability of selecting a bad pivot value and
      eliminates certain extraneous comparisons.

   3. Only quicksorts TOTAL_ELEMS / MAX_THRESH partitions, leaving
      insertion sort to order the MAX_THRESH items within each partition.
      This is a big win, since insertion sort is faster for small, mostly
      sorted array segments.

   4. The larger of the two sub-partitions is always pushed onto the
      stack first, with the algorithm then concentrating on the
      smaller partition.  This *guarantees* no more than log (n)
      stack size is needed (actually O(1) in this case)!  */

static __always_inline __flatten int
qsort_template (const struct qsort_def *def, void *const pbase,
                size_t n, void *arg)
{
  /* this copy of def will be mutated to manage indirect sorting if needed */
  struct qsort_def d = *def;
  char *base_ptr = (char *) pbase;
  size_t max_thresh;

  /* The real size allocated is MAX_TMP_STACK_SIZE + d.align - 1. This space is
   * in addition to the 256 to 768 (or 1024 if uncapped) bytes used in the
   * primary loop. */
  const size_t MAX_TMP_STACK_SIZE = 512;
  int elem_buf_on_heap = 0;        /* ct const */
  int index_on_heap    = 0;        /* ct const */
  /* Use indirect sorting if size is large */
  int indirect         = d.size > _QSORT_IND_THRESH; /* ct const */
  size_t tmp_needed    = 0;        /* both (can result in either ct or rt value) */
  int ret              = 0;

  if (n == 0)
    /* Avoid lossage with unsigned arithmetic below.  */
    return 0;

  /* Restrict to reasonable value */
  if (d.align > _QSORT_ALIGN_MAX)
    d.align = _QSORT_ALIGN_MAX;

  if (!d.max_size_bits)
    d.max_size_bits = sizeof(size_t) * 8 * 4 / 3;

  if (!d.max_thresh)
    d.max_thresh = 4;

  if (!d.max_stack_space)
    d.max_stack_space = 1024;


  assert_const(!!d.less);
  assert_const(d.align + d.size);
  BUILD_BUG_ON_MSG(!d.less, "less function is required");

#if __STDC_VERSION__ >= 201112L
  BUILD_BUG_ON_MSG(_Alignof(max_align_t) & (_Alignof(max_align_t) - 1),
                   "_Alignof(max_align_t) is not a power of two");
#endif

  BUILD_BUG_ON_MSG(d.align & (d.align - 1), "align must be a power of two");
  BUILD_BUG_ON_MSG(d.size % d.align, "size must be a multiple of align");

  /* verify pbase is really aligned as advertised */
  assert_early(!((uintptr_t)pbase & (d.align - 1)));

  if (!d.elem_buf)
    tmp_needed += def->size;

  if (d.size > _QSORT_IND_THRESH || !d.index)
    tmp_needed += sizeof(void *) * n;

  /* if we don't already have a temp element buffer allocate one now */
  if (!d.elem_buf)
    {
      /* try to get an aligned d.elem_buf buffer the as large as size */
      if (d.size > MAX_TMP_STACK_SIZE)
        {
          /* FIXME: pre C11 compat */
          d.elem_buf = aligned_alloc(d.align, d.size);
          elem_buf_on_heap = 1;

          if (!d.elem_buf)
            return ENOMEM;
        }
      else
        {
          size_t offset;
          /* put it on the stack */
          d.elem_buf = alloca (d.size + d.align - 1);

          /* align it */
          offset = (uintptr_t)d.elem_buf % d.align;
          if (offset)
            d.elem_buf = (char *)d.elem_buf + d.align - offset;
        }
    }

  if (indirect)
    {
      size_t i;

      d.size = sizeof(void *);
      d.align = _Alignof(void *);

      if (!d.index)
        {
          index_on_heap = 1;
          d.index = malloc(sizeof(void *) * n);
          if (!d.index)
            {
              ret = ENOMEM;
              goto exit_free_elem_buf;
            }
        }

      assert(!((uintptr_t)d.index & (d.align - 1)));

      for (i = n; i--;)
        d.index[i] = base_ptr + i * def->size;
      base_ptr = (char *) d.index;

//  printf("pbase = %p, base_ptr = %p, d.elem_buf = %p, d.index = %p\n", pbase, base_ptr, d.elem_buf, d.index);
    }

  /* now that we're certain about d.size... */
  max_thresh = MAX_THRESH * d.size;


  /* These locals should still be compile-time constants */
  assert_const(elem_buf_on_heap);
  assert_const(index_on_heap);
  assert_const(indirect);
  assert_const(max_thresh);

  if (n > MAX_THRESH)
    {
      char *lo = base_ptr;
      char *hi = &lo[d.size * (n - 1)];
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

          char *mid = lo + d.size * ((hi - lo) / d.size >> 1);

          if (_quicksort_less (&d, indirect, (void *) mid, (void *) lo, arg))
            _quicksort_swap (&d, mid, lo);
          if (_quicksort_less (&d, indirect, (void *) hi, (void *) mid, arg))
            _quicksort_swap (&d, mid, hi);
          else
            goto jump_over;
          if (_quicksort_less (&d, indirect, (void *) mid, (void *) lo, arg))
            _quicksort_swap (&d, mid, lo);
        jump_over:
          ;

          left_ptr  = lo + d.size;
          right_ptr = hi - d.size;

          /* Here's the famous ``collapse the walls'' section of quicksort.
             Gotta like those tight inner loops!  They are the main reason
             that this algorithm runs much faster than others. */
          do
            {
              while (_quicksort_less (&d, indirect, (void *) left_ptr, (void *) mid, arg))
                left_ptr += d.size;

              while (_quicksort_less (&d, indirect, (void *) mid, (void *) right_ptr, arg))
                right_ptr -= d.size;

              if (left_ptr < right_ptr)
                {
                  _quicksort_swap (&d, left_ptr, right_ptr);
                  if (mid == left_ptr)
                    mid = right_ptr;
                  else if (mid == right_ptr)
                    mid = left_ptr;
                  left_ptr += d.size;
                  right_ptr -= d.size;
                }
              else if (left_ptr == right_ptr)
                {
                  left_ptr += d.size;
                  right_ptr -= d.size;
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


  /* if element size is a power of two, indexed addressing will be more
   * efficient in most cases */
  if (d.size <= _QSORT_ARCH_MAX_INDEX_MULT || !(d.size & (d.size - 1)))
    {
      const size_t thresh = min (n, MAX_THRESH + 1);
      size_t left, right;
      void *smallest;

      /* Find smallest element in first threshold and place it at the
          array's beginning.  This is the smallest array element,
          and the operation speeds up insertion sort's inner loop. */

      for (smallest = base_ptr, right = 1; right < thresh; ++right)
        {
          char *p = base_ptr + right * d.size;

          if (_quicksort_less (&d, indirect, p, smallest, arg))
            smallest = p;
        }

      if (smallest != base_ptr)
        _quicksort_swap (&d, smallest, base_ptr);

      for (right = 2; right < n; ++right)
        {
          left = right - 1;
          while (_quicksort_less (&d, indirect, &base_ptr[right * d.size], &base_ptr[left * d.size], arg))
            {
              assert(left);
              --left;
            }

          ++left;
          if (left != right)
            _quicksort_ror(&d, &base_ptr[left * d.size], &base_ptr[right * d.size]);
        }
    }
  else
    {
      /* if not a power of two, use ptr arithmetic */
      char *const end_ptr = &base_ptr[d.size * (n - 1)];
      char *tmp_ptr = base_ptr;
      char *thresh = min(end_ptr, base_ptr + max_thresh);
      register char *run_ptr;

      /* Find smallest element in first threshold and place it at the
         array's beginning.  This is the smallest array element,
         and the operation speeds up insertion sort's inner loop. */

      for (run_ptr = tmp_ptr + d.size; run_ptr <= thresh; run_ptr += d.size)
        if (_quicksort_less (&d, indirect, (void *) run_ptr, (void *) tmp_ptr, arg))
          tmp_ptr = run_ptr;

      if (tmp_ptr != base_ptr)
        _quicksort_swap (&d, tmp_ptr, base_ptr);

      /* Insertion sort, running from left-hand-side up to right-hand-side.  */

      run_ptr = base_ptr + d.size;
      while ((run_ptr += d.size) <= end_ptr)
        {
          tmp_ptr = run_ptr - d.size;
          while (_quicksort_less (&d, indirect, (void *) run_ptr, (void *) tmp_ptr, arg))
            tmp_ptr -= d.size;

          tmp_ptr += d.size;
          if (tmp_ptr != run_ptr)
            _quicksort_ror(&d, tmp_ptr, run_ptr);
        }
    }


  /* if we used indirect sorting, now we have to re-arrange the array. */
  if (indirect)
    {
      size_t i;		// current element (indecies)
      char *ip;		// pointer to the current element
      char *kp;

      for (i = 0, ip = (char *)pbase; i < n; ++i, ip += def->size)
        {
          if ((kp = d.index[i]) != ip)
            {
              size_t j = i;
              char *jp = ip;
              _quicksort_copy (def, d.elem_buf, ip);

              do
                {
                  size_t k = (kp - (char *)pbase) / def->size;
                  d.index[j] = jp;
                  _quicksort_copy (def, jp, kp);
                  j = k;
                  jp = kp;
                  kp = d.index[k];
                }
              while (kp != ip);

              d.index[j] = jp;
              _quicksort_copy (def, jp, d.elem_buf);
            }
        }
    }

  if (index_on_heap)
    free (d.index);

exit_free_elem_buf:
  if (elem_buf_on_heap)
    free (d.elem_buf);

  return ret;
}

#endif /* _QSORT_H_ */
