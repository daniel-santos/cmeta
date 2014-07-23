/*
 *
 * Some utility macros and functions lazily put into static form */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <error.h>
#include <errno.h>
#include <assert.h>

#include "compiler.h"

#if 0
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#endif

#define _do_error(fatal, err, file, line, fmt, ...)			\
	error_at_line(fatal, err, file, line, "%s - " fmt,		\
		      __PRETTY_FUNCTION__, ## __VA_ARGS__)
#define fatal_error(fmt, ...)						\
	_do_error(1, errno, __FILE__, __LINE__, fmt, ## __VA_ARGS__)

#ifndef likely
# define likely(x)		__builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
# define unlikely(x)		__builtin_expect(!!(x), 0)
#endif


static void randomize(void *p, size_t n, size_t size, unsigned int seed) {
	unsigned long *arr = p;
	const size_t LONG_BITS = sizeof(unsigned long) * 8;
	const size_t RAND_BITS = LONG_BITS - __builtin_clzl((unsigned long)RAND_MAX);
	const size_t bytes = n * size;
	const size_t count = bytes / sizeof(unsigned long);
	size_t i;

	//assert(!(bytes % sizeof(*arr)));
	//assert(size == sizeof(*arr));

	srandom(seed);

	for (i = 0; i < count; ++i) {
		size_t bits;

		arr[i] = (unsigned long)random();

		for (bits = RAND_BITS; bits < LONG_BITS; bits += RAND_BITS) {
			arr[i] <<= RAND_BITS;
			arr[i] ^= (unsigned long)random();
		}
//		arr[i] >>= 56;
	}

	/* if not aligned to size of long get the last few bytes */
	for (i = 0; i < bytes % sizeof(*arr); ++i) {
		((char *)p) [count * sizeof(*arr) + i] = random();
	}
}

static inline void timespec_set(struct timespec *ts) {
	if (unlikely(errno = clock_gettime(CLOCK_THREAD_CPUTIME_ID, ts)))
		fatal_error("clock_gettime");
}

static inline struct timespec timespec_subtract(struct timespec a, struct timespec b) {
	const long ONE_BILLION = 1000000000ul;
	struct timespec ret = {
		.tv_sec  = a.tv_sec  - b.tv_sec,
		.tv_nsec = a.tv_nsec - b.tv_nsec,
	};

	if (ret.tv_nsec < 0) {
		ret.tv_sec  -= 1;
		ret.tv_nsec += ONE_BILLION;
	}

	return ret;
}

static inline struct timespec timespec_add(struct timespec a, struct timespec b) {
	const long ONE_BILLION = 1000000000ul;
	struct timespec ret = {
		.tv_sec  = a.tv_sec  + b.tv_sec,
		.tv_nsec = a.tv_nsec + b.tv_nsec,
	};

	if (ret.tv_nsec >= ONE_BILLION) {
		ret.tv_sec  += 1;
		ret.tv_nsec -= ONE_BILLION;
	}

	return ret;
}

static double time_pct(struct timespec *a, struct timespec *b) {
	const double ONE_BILLION = 1000000000.;
	double da = (double)a->tv_nsec + ONE_BILLION *a->tv_sec;
	double db = (double)b->tv_nsec + ONE_BILLION *b->tv_sec;
	return ((da / db) - 1.) * 100.;
}

#endif /* _UTILS_H_ */
