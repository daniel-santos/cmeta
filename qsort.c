//#define _GNU_SOURCE

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

static __always_inline int my_cmp(const void *a, const void *b, void *context) {
	return *(long*)a - *(long*)b;
}

static __always_inline void my_copy(const struct qsort_def *def, void *dest, const void *src) {
	long       __aligned(sizeof(long)) *d = dest;
	const long __aligned(sizeof(long)) *s = src;
	*d = *s;
}

static __always_inline void my_swap(const struct qsort_def *def, void *a, void *b) {
	long __aligned(sizeof(long)) tmp;
	my_copy(def, &tmp, a);
	my_copy(def, a, b);
	my_copy(def, b, &tmp);
}

static const struct qsort_def my_def = {
	.size = 8,
	.align = sizeof(long),
	.cmp = my_cmp,
	.copy = my_copy,
	.swap = my_swap,
};

static const size_t ARRAY_COUNT = 1024 * 16;

/* using static noinline to make it easier to examine generated code */
static __noinline __flatten void my_quicksort(long *arr) {
	_quicksort_template(&my_def, arr, ARRAY_COUNT, NULL);
}

int main(int argc, char **argv) {
	long *arr;
	size_t i;

	struct timespec start, end;
	struct timespec total = {0, 0};

	arr = malloc(sizeof(long) * ARRAY_COUNT);
	if (unlikely(!arr)) {
		errno = ENOMEM;
		fatal_error("malloc %lu bytes\n", sizeof(long) * ARRAY_COUNT);
	}

	assert (!((uintptr_t)arr & 7));
	srandom(0);
	for (i = 0; i < 1024; ++i) {

		randomize(arr, ARRAY_COUNT, 0);
		timespec_set(&start);

		_quicksort(arr, ARRAY_COUNT, sizeof(long), my_cmp, NULL);

		timespec_set(&end);
		total = timespec_add(total, timespec_subtract(end, start));
	}

	printf("gnu _quicksort             = %02lu:%02lu.%09lu\n", total.tv_sec / 60, total.tv_sec % 60, total.tv_nsec);

	total.tv_sec = 0;
	total.tv_nsec = 0;

	for (i = 0; i < 1024; ++i) {

		randomize(arr, ARRAY_COUNT, 0);
		timespec_set(&start);

		// _quicksort_instantiate(arr, ARRAY_COUNT, sizeof(long), 8, long_cmp, NULL);
		my_quicksort(arr);

		timespec_set(&end);
		total = timespec_add(total, timespec_subtract(end, start));
	}

	printf("_quicksort_instantiate = %02lu:%02lu.%09lu\n", total.tv_sec / 60, total.tv_sec % 60, total.tv_nsec);

	return 0;
}

