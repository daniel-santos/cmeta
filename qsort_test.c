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

#define  _ISOC11_SOURCE
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
# define ELEM_SIZE 4096
#endif

#ifndef ALIGN_SIZE
# define ALIGN_SIZE 4
#endif

#ifndef NUM_ELEMS
# define NUM_ELEMS 4
#endif

#ifndef TEST_COUNT
# define TEST_COUNT (1024 * 128)
#endif

#ifndef KEY_SIGN
# define KEY_SIGN uint
#endif

#define key_type(bits) KEY_SIGN ## bits ## _t

static __always_inline int my_cmp(const void *a, const void *b, void *context) {
	if (ELEM_SIZE == 1) {
		const uint8_t *_a = __builtin_assume_aligned(a, ALIGN_SIZE);
		const uint8_t *_b = __builtin_assume_aligned(b, ALIGN_SIZE);
		return *_a - *_b;
	} else if (ELEM_SIZE == 2) {
		const uint16_t *_a = __builtin_assume_aligned(a, ALIGN_SIZE);
		const uint16_t *_b = __builtin_assume_aligned(b, ALIGN_SIZE);
		return *_a - *_b;
	} else if (ELEM_SIZE < 8) {
		const uint32_t *_a = __builtin_assume_aligned(a, ALIGN_SIZE);
		const uint32_t *_b = __builtin_assume_aligned(b, ALIGN_SIZE);
		return *_a == *_b ? 0 : (*_a > *_b ? 1 : -1);
	} else {
		const uint64_t *_a = __builtin_assume_aligned(a, ALIGN_SIZE);
		const uint64_t *_b = __builtin_assume_aligned(b, ALIGN_SIZE);
		return *_a == *_b ? 0 : (*_a > *_b ? 1 : -1);
	}
}

static __always_inline int my_less(const void *a, const void *b, void *context) {
	if (ELEM_SIZE == 1) {
		const uint8_t *_a = __builtin_assume_aligned(a, ALIGN_SIZE);
		const uint8_t *_b = __builtin_assume_aligned(b, ALIGN_SIZE);
		return *_a < *_b;
	} else if (ELEM_SIZE == 2) {
		const uint16_t *_a = __builtin_assume_aligned(a, ALIGN_SIZE);
		const uint16_t *_b = __builtin_assume_aligned(b, ALIGN_SIZE);
		return *_a < *_b;
	} else if (ELEM_SIZE < 8) {
		const uint32_t *_a = __builtin_assume_aligned(a, ALIGN_SIZE);
		const uint32_t *_b = __builtin_assume_aligned(b, ALIGN_SIZE);
		return *_a < *_b;
	} else {
		const uint64_t *_a = __builtin_assume_aligned(a, ALIGN_SIZE);
		const uint64_t *_b = __builtin_assume_aligned(b, ALIGN_SIZE);
		return *_a < *_b;
	}
}

/* packing isn't normally something one would do, but we need to force the
 * specified object size, so we use pack(1) to get that */
#pragma pack(push,1)
struct size_type {
	char c[ELEM_SIZE];
};
#pragma pack(pop)

static const struct qsort_def my_def = {
	.size = ELEM_SIZE,
	.align = ALIGN_SIZE,
	.less = my_less,
};


typedef void (*sort_func_t)(void *p, size_t n, size_t size, int (*compar)(const void *, const void *, void *arg), void *arg);

/* using static noinline to make it easier to examine generated code */
static __noinline __flatten void my_quicksort(void *p, size_t n, size_t elem_size, int (*compar)(const void *, const void *, void *arg), void *arg) {
	int ret = qsort_template(&my_def, p, n, NULL);

	if (ret)
		fatal_error("qsort_template returned %d\n", ret);
}

static void dump_keys(void * const data[4], size_t n, const char *heading) {
	size_t i;

	fprintf(stderr, "\n%s:\n", heading);
	if (ELEM_SIZE == 1) {
		const uint8_t **p = (const uint8_t **)data;

		fprintf(stderr, "%-9s %-5s %-5s %-5s %-5s\n",
				"offset", "orig", "mine", "qsort", "msort");
		for (i = 0; i < n; ++i)
			fprintf(stderr, "%08lx: %02hhx    %02hhx    %02hhx    %02hhx\n",
				i * sizeof(**p),
				p[0][i], p[1][i], p[2][i], p[3][i]);
	} else if (ELEM_SIZE == 2) {
		const uint16_t **p = (const uint16_t **)data;

		fprintf(stderr, "%-9s %-5s %-5s %-5s %-5s\n",
				"offset", "orig", "mine", "qsort", "msort");
		for (i = 0; i < n; ++i)
			fprintf(stderr, "%08lx: %04hx  %04hx  %04hx  %04hx\n",
				i * sizeof(**p),
				p[0][i], p[1][i], p[2][i], p[3][i]);
	} else if (ELEM_SIZE == 4) {
		const uint32_t **p = (const uint32_t **)data;

		fprintf(stderr, "%-9s %-8s %-8s %-8s %-8s\n",
				"offset", "orig", "mine", "qsort", "msort");
		for (i = 0; i < n; ++i)
			fprintf(stderr, "%08lx: %08x %08x %08x %08x\n",
				i * sizeof(**p),
				p[0][i], p[1][i], p[2][i], p[3][i]);
	} else if (ELEM_SIZE >= 8) {
		const char **p = (const char **)data;

		fprintf(stderr, "%-9s %-16s %-16s %-16s %-16s\n",
				"offset", "orig", "mine", "qsort", "msort");
		for (i = 0; i < n; ++i)
			fprintf(stderr, "%08lx: %016lx %016lx %016lx %016lx\n",
				i * sizeof(**p),
                                *(uint64_t*)(&p[0][i * ELEM_SIZE]),
                                *(uint64_t*)(&p[1][i * ELEM_SIZE]),
                                *(uint64_t*)(&p[2][i * ELEM_SIZE]),
                                *(uint64_t*)(&p[3][i * ELEM_SIZE]));
	}
}

/* Make sure that _quicksort_template() is correct given these parameters */
void validate_sort(size_t n, size_t elem_size, size_t min_align, unsigned int seed) {
	void *data[4];
	const char *algo_desc[4] = {"orig", "my_quicksort", "_quicksoft", "qsort_r"};
	const size_t DATA_SIZE = sizeof(data) / sizeof(*data);
	size_t bytes = n * elem_size;
	size_t i;

	BUILD_BUG_ON(sizeof(struct size_type) != ELEM_SIZE);

	/* allocate buffers */
	for (i = 0; i < DATA_SIZE; ++i) {
		data[i] = aligned_alloc(min_align, bytes);
		assert(!((uintptr_t)data[i] & (min_align - 1)));
	}

	/* randomize first buffer */
	randomize(data[0], n, elem_size, seed);

	/* and copy to the other buffers */
	for (i = 1; i < DATA_SIZE; ++i) {
		memcpy(data[i], data[0], bytes);
		assert(!memcmp(data[i], data[0], bytes));
	}

	if (0)
		dump_keys(data[0], n, "BEFORE");

	my_quicksort(data[1], n, elem_size, my_cmp, NULL);
	_quicksort  (data[2], n, elem_size, my_cmp, NULL);
	qsort_r     (data[3], n, elem_size, my_cmp, NULL);

#if 0
	/* un-comment to induce error */
	*((int*)data[1]) -= 1;
#endif

	/* compare result of my_quicksort against results of other algos */
	for (i = 2; i < DATA_SIZE - 1; ++i) {
		if (memcmp(data[1], data[i], bytes)) {
			dump_keys(data, n, "");
			fprintf(stderr, "\n");
			fatal_error("\nmy_quicksort produced different result than %s", algo_desc[i]);
		}
	}

	for (i = 1; i < DATA_SIZE; ++i)
		free (data[i]);
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

	/* verify that we have forced the object to whatever size we've
	 * specified, even if it's stupid */
	BUILD_BUG_ON(sizeof(struct size_type) != ELEM_SIZE);

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

	validate_sort(NUM_ELEMS, ELEM_SIZE, ALIGN_SIZE, 0);

	arr = aligned_alloc(ALIGN_SIZE, ELEM_SIZE * (size_t)NUM_ELEMS);
	if (unlikely(!arr)) {
		errno = ENOMEM;
		fatal_error("malloc %lu bytes\n", ELEM_SIZE * (size_t)NUM_ELEMS);
	}

	qsort  = run_test(arr, NUM_ELEMS, ELEM_SIZE, ALIGN_SIZE, 0, TEST_COUNT, _quicksort, "_quicksort");
	msort  = run_test(arr, NUM_ELEMS, ELEM_SIZE, ALIGN_SIZE, 0, TEST_COUNT, qsort_r, "qsort_r");
	mysort = run_test(arr, NUM_ELEMS, ELEM_SIZE, ALIGN_SIZE, 0, TEST_COUNT, my_quicksort, "my_quicksort");

	printf("%.2f%% faster than _quicksort\n", time_pct(&qsort, &mysort));
	printf("%.2f%% faster than qsort_r\n", time_pct(&msort, &mysort));

	printf("\n\nn, elem_size, min_align, total_bytes, repeat_count, cmp_to_qsort, cmp_to_msort, time_qsort, time_msort, time_mysort\n");
	printf("%lu, %lu, %lu, %lu, %lu, %.2f%%, %.2f%%, %lu.%09lu, %lu.%09lu, %lu.%09lu\n",
		   (size_t)NUM_ELEMS, (size_t)ELEM_SIZE, (size_t)ALIGN_SIZE,
		   (size_t)ELEM_SIZE * (size_t)NUM_ELEMS, (size_t)TEST_COUNT,
		   time_pct(&qsort, &mysort), time_pct(&msort, &mysort),
		   qsort.tv_sec, qsort.tv_nsec,
		   msort.tv_sec, msort.tv_nsec,
		   mysort.tv_sec, mysort.tv_nsec);

	free (arr);
	return 0;
}

