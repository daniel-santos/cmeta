/*
 * static_strlen - demo for qsort implemented as a C pseudo-template function
 * Copyright (C) 2014 Daniel Santos <daniel.santos@pobox.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */
//#define  _ISOC11_SOURCE
#define _GNU_SOURCE

#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <error.h>
#include <assert.h>

#include "qsort.h"
#include "utils.h"

#ifndef ELEM_SIZE
# define ELEM_SIZE 8
#endif

#ifndef ALIGN_SIZE
# define ALIGN_SIZE 8
#endif

#ifndef NUM_ELEMS
# define NUM_ELEMS 16
#endif

#ifndef TEST_COUNT
# define TEST_COUNT 1
#endif

static __always_inline int my_cmp(const void *a, const void *b, void *context) {
	if (ELEM_SIZE == 1) {
		const int8_t __aligned(ALIGN_SIZE) *_a = a;
		const int8_t __aligned(ALIGN_SIZE) *_b = b;
		return *_a - *_b;
	} else if (ELEM_SIZE < 4) {
		const int16_t __aligned(ALIGN_SIZE) *_a = a;
		const int16_t __aligned(ALIGN_SIZE) *_b = b;
		return *_a - *_b;
	} else if (ELEM_SIZE < 8) {
		const int32_t __aligned(ALIGN_SIZE) *_a = a;
		const int32_t __aligned(ALIGN_SIZE) *_b = b;
		return *_a - *_b;
	} else {
		const int64_t __aligned(ALIGN_SIZE) *_a = a;
		const int64_t __aligned(ALIGN_SIZE) *_b = b;
		return *_a == *_b ? 0 : (*_a > *_b ? 1 : -1);
	}
}

struct size_type {
	char c[ELEM_SIZE];
};

static __always_inline void my_copy(const struct qsort_def *def, void *dest, const void *src) {
	const struct size_type __aligned(ALIGN_SIZE) *s = src;
	struct size_type __aligned(ALIGN_SIZE) *d = dest;
//        fprintf(stderr, "copy: d=%p, s=%p\n", d, s);
	*d = *s;
}

static __always_inline __flatten void my_swap(const struct qsort_def *def, void *a, void *b) {
	struct size_type __aligned(ALIGN_SIZE) tmp;
	struct size_type __aligned(ALIGN_SIZE) *_a = a;
	struct size_type __aligned(ALIGN_SIZE) *_b = b;
	my_copy(def, &tmp, _a);
	my_copy(def, _a, _b);
	my_copy(def, _b, &tmp);
}

static const struct qsort_def my_def = {
	.size = ELEM_SIZE,
	.align = ALIGN_SIZE,
	.cmp = my_cmp,
	.copy = my_copy,
	.swap = my_swap,
};


typedef void (*sort_func_t)(void *p, size_t n, size_t size, int (*compar)(const void *, const void *, void *arg), void *arg);

/* using static noinline to make it easier to examine generated code */
static __noinline __flatten void my_quicksort(void *p, size_t n, size_t elem_size, int (*compar)(const void *, const void *, void *arg), void *arg) {
	_quicksort_template(&my_def, p, NUM_ELEMS, NULL);
}

static void dump_longs(const void *data, size_t n, const char *heading) {
	const long *p = data;
	size_t i;

	fprintf(stderr, "\n%s:\n", heading);
	for (i = 0; i < n; ++i)
		fprintf(stderr, "0x%08lx 0x%016lx\n", i * sizeof(long), p[i]);
}

void validate_sort(void *p, size_t n, size_t elem_size, size_t min_align, unsigned int seed) {
	void *copy;
	size_t bytes = n * elem_size;

	assert(!((uintptr_t)p & (min_align - 1)));
	BUILD_BUG_ON(sizeof(struct size_type) != ELEM_SIZE);

	copy = aligned_alloc(min_align, bytes);
	assert(!((uintptr_t)copy & (min_align - 1)));

	srandom(2);

	randomize(p, n, elem_size, random());
	memcpy(copy, p, bytes);

	if (memcmp(p, copy, bytes)) {
		fatal_error("something is screwy");
	}

	dump_longs(p, n, "BEFORE");

	my_quicksort(p, n, elem_size, my_cmp, NULL);
	_quicksort(copy, n, elem_size, my_cmp, NULL);

	if (memcmp(p, copy, bytes)) {
		fprintf(stderr, "\nmy_quicksort output:");
	dump_longs(p, n, "mine");
	dump_longs(copy, n, "qsort_r");
		fprintf(stderr, "\n");
		fatal_error("\nmy_quicksort produced different result than qsort_r");
	}

	free (copy);
}

static void print_result(struct timespec *total, const char *desc) {
	printf("%16s = %02lu:%02lu.%09lu\n", desc, total->tv_sec / 60, total->tv_sec % 60, total->tv_nsec);
}

struct timespec run_test(void *p, size_t n, size_t elem_size, size_t min_align, unsigned int seed, unsigned test_count, sort_func_t sortfn, const char *desc) {
	struct timespec start, end;
	struct timespec total = {0, 0};
	size_t i;

	assert_early(!((uintptr_t)p & (min_align - 1)));

#if 0
	printf("\n\nRunning test %s:\n"
		   "n            = %lu\n"
		   "elem_size    = %lu\n"
		   "min_align    = %lu\n"
		   "total bytes  = %lu\n"
		   "repeat_count = %u\n",
		   desc, n, elem_size, min_align, n * elem_size, test_count);
#endif

	srandom(0);
	for (i = test_count; i ; --i) {

		randomize(p, n, elem_size, random());
		timespec_set(&start);

		sortfn(p, n, elem_size, my_cmp, NULL);

		timespec_set(&end);
		total = timespec_add(total, timespec_subtract(end, start));
	}

	print_result(&total, desc);

	return total;
}

int main(int argc, char **argv) {
	void *arr;
	struct timespec qsort, msort, mysort;


	arr = aligned_alloc(ALIGN_SIZE, ELEM_SIZE * (size_t)NUM_ELEMS);
	if (unlikely(!arr)) {
		errno = ENOMEM;
		fatal_error("malloc %lu bytes\n", ELEM_SIZE * (size_t)NUM_ELEMS);
	}

//	fprintf(stderr, "%lu %lu %ld", sizeof(max_align_t), _Alignof(long), __STDC_VERSION__);//, sizeof(max_align_t));
//	exit(1);

	printf("\n\nRunning tests with:\n"
		   "n            = %lu\n"
		   "elem_size    = %lu\n"
		   "min_align    = %lu\n"
		   "total bytes  = %lu\n"
		   "repeat_count = %lu\n",
		   (size_t)NUM_ELEMS, (size_t)ELEM_SIZE, (size_t)ALIGN_SIZE,
		   (size_t)ELEM_SIZE * (size_t)NUM_ELEMS, (size_t)TEST_COUNT);

	validate_sort(arr, NUM_ELEMS, ELEM_SIZE, ALIGN_SIZE, 0);
exit(1);
	qsort  = run_test(arr, NUM_ELEMS, ELEM_SIZE, ALIGN_SIZE, 0, TEST_COUNT, _quicksort, "_quicksort");
	msort  = run_test(arr, NUM_ELEMS, ELEM_SIZE, ALIGN_SIZE, 0, TEST_COUNT, qsort_r, "qsort_r");
	mysort = run_test(arr, NUM_ELEMS, ELEM_SIZE, ALIGN_SIZE, 0, TEST_COUNT, my_quicksort, "my_quicksort");

	printf("%.2f%% faster than _quicksort\n", time_pct(&qsort, &mysort));
	printf("%.2f%% faster than qsort_r\n", time_pct(&msort, &mysort));

	printf("\n\nn, elem_size, min_align, total_bytes, repeat_count, cmp_to_qsort, cmp_to_msort\n");
	printf("%lu, %lu, %lu, %lu, %lu, %.2f%%, %.2f%%\n",
		   (size_t)NUM_ELEMS, (size_t)ELEM_SIZE, (size_t)ALIGN_SIZE,
		   (size_t)ELEM_SIZE * (size_t)NUM_ELEMS, (size_t)TEST_COUNT,
		   time_pct(&qsort, &mysort), time_pct(&msort, &mysort));

	free (arr);
	return 0;
}

