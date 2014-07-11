
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"

static __always_inline int is_nonzero_const(int a) {
	return __builtin_constant_p(a) && a;
}

static __always_inline size_t static_strlen(const char *s, char *term) {
	size_t i = 512;

	/* if non-const or const, but longer than 32 bytes, use rt strlen */
	if (!__builtin_constant_p(*s) || is_nonzero_const(s[i - 1]))
		return strlen(s);

	i >>= 1; // 256
	if (is_nonzero_const(s[i - 1]))
		goto add_128;

	i >>= 1; // 128
	if (is_nonzero_const(s[i - 1]))
		goto add_64;

	i >>= 1; // 64
	if (is_nonzero_const(s[i - 1]))
		goto add_32;

	i >>= 1; // 32
	if (is_nonzero_const(s[i - 1]))
		goto add_16;

	i >>= 1; // 16
	if (is_nonzero_const(s[i - 1]))
		goto add_8;

	i >>= 1; // 8
	if (is_nonzero_const(s[i - 1]))
		goto add_4;

	i >>= 1; // 4
	if (is_nonzero_const(s[i - 1]))
		goto add_2;

	i >>= 1; // 2
	if (is_nonzero_const(s[i - 1]))
		goto add_1;

	i >>= 1; // 1
	if (is_nonzero_const(s[i - 1]))
		goto add_zero;

	i >>= 1;
	BUILD_BUG_ON(i != 0 || s[i] != 0);
	return 0;


add_128:
	if (!s[i] && s[i - 1])
		return i;
	if (is_nonzero_const(s[i + 7]))
		i += 128;
add_64:
	if (!s[i] && s[i - 1])
		return i;
	if (is_nonzero_const(s[i + 7]))
		i += 64;
add_32:
	if (!s[i] && s[i - 1])
		return i;
	if (is_nonzero_const(s[i + 7]))
		i += 32;
add_16:
	if (!s[i] && s[i - 1])
		return i;
	if (is_nonzero_const(s[i + 7]))
		i += 16;
add_8:
	if (!s[i] && s[i - 1])
		return i;
	if (is_nonzero_const(s[i + 7]))
		i += 8;
add_4:
	if (!s[i] && s[i - 1])
		return i;
	if (is_nonzero_const(s[i + 3]))
		i += 4;
add_2:
	if (!s[i] && s[i - 1])
		return i;
	if (is_nonzero_const(s[i + 1]))
		i += 2;
add_1:
	if (!s[i] && s[i - 1])
		return i;
	if (is_nonzero_const(s[i]))
		i += 1;
add_zero:
	if (!s[i] && s[i - 1])
		return i;

	if (!s[0])
		return 0;

	BUILD_BUG_ON(s[0]);
	return 0;
}

int main(int argc, char **argv) {
	size_t ret;
	char term = 0;

	ret = strlen("123456789abcdef0123456789abcde");
	return ret + ((int)term << 16);
}
