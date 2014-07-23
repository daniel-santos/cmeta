//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>

//#include "compiler.h"
//#include "utils.h"
#include <stdint.h>
#include <assert.h>

#ifndef ELEM_SIZE
# define ELEM_SIZE 2048
#endif

struct asdf {
	char c[ELEM_SIZE];
};

#if 1
void copy_data_1(void *dest, void *src) {
	struct asdf *d = __builtin_assume_aligned(dest, 1);
	struct asdf *s = __builtin_assume_aligned(src, 1);
	*d = *s;
}

void copy_data_2(void *dest, void *src) {
	struct asdf *d = __builtin_assume_aligned(dest, 2);
	struct asdf *s = __builtin_assume_aligned(src, 2);
	*d = *s;
}

void copy_data_4(void *dest, void *src) {
	struct asdf *d = __builtin_assume_aligned(dest, 4);
	struct asdf *s = __builtin_assume_aligned(src, 4);
	*d = *s;
}

void copy_data_8(void *dest, void *src) {
	struct asdf *d = __builtin_assume_aligned(dest, 8);
	struct asdf *s = __builtin_assume_aligned(src, 8);
	*d = *s;
}

void copy_data_16(void *dest, void *src) {
	struct asdf *d = __builtin_assume_aligned(dest, 16);
	struct asdf *s = __builtin_assume_aligned(src, 16);
	*d = *s;
}

void copy_data_32(void *dest, void *src) {
	struct asdf *d = __builtin_assume_aligned(dest, 16);
	struct asdf *s = __builtin_assume_aligned(src, 16);
	*d = *s;
}

#else
	if (!((uintptr_t)dest & 15ul) && !((uintptr_t)src & 15ul))

void copy_data_1(void *dest, void *src) {
	struct asdf _Alignas(1) *d = dest;
	struct asdf _Alignas(1) *s = src;
	*d = *s;
}

void copy_data_2(void *dest, void *src) {
	struct asdf _Alignas(2) *d = dest;
	struct asdf _Alignas(2) *s = src;
	*d = *s;
}

void copy_data_4(void *dest, void *src) {
	struct asdf _Alignas(4) *d = dest;
	struct asdf _Alignas(4) *s = src;
	*d = *s;
}

void copy_data_8(void *dest, void *src) {
	struct asdf _Alignas(8) *d = dest;
	struct asdf _Alignas(8) *s = src;
	*d = *s;
}

void copy_data_16(void *dest, void *src) {
	struct asdf _Alignas(16) *d = dest & ~15;
	struct asdf _Alignas(16) *s = src & ~15;
	*d = *s;
}

void copy_data_32(void *dest, void *src) {
	struct asdf _Alignas(32) *d = dest;
	struct asdf _Alignas(32) *s = src;
	*d = *s;
}

#endif
