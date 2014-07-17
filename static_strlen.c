/*
 * static_strlen - example compile-time strlen implementation
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"

static __always_inline int is_nonzero_const(int v) {
	return __builtin_constant_p(v) && v;
}

#define _STATIC_STRLEN_DEC(s, i, n)			\
	do {									\
		i >>= 1;							\
		if (is_nonzero_const(s[i - 1]))		\
			goto add_ ## n;					\
	} while (0)

#define _STATIC_STRLEN_INC(s, i, n)			\
	do {									\
add_ ## n:									\
		if (!s[i] && s[i - 1])				\
			return i;						\
		if (is_nonzero_const(s[i + n - 1]))	\
			i += n;							\
	} while (0)

static __always_inline size_t static_strlen(const char *s) {
	size_t i = 65536;

	/* if non-const or const, but longer than 64k bytes, use rt strlen */
	if (!__builtin_constant_p(*s) || is_nonzero_const(s[i - 1])) {
		BUILD_BUG();
		//return strlen(s);
	}

	_STATIC_STRLEN_DEC(s, i, 16384);
	_STATIC_STRLEN_DEC(s, i, 8192);
	_STATIC_STRLEN_DEC(s, i, 4096);
	_STATIC_STRLEN_DEC(s, i, 2048);
	_STATIC_STRLEN_DEC(s, i, 1024);
	_STATIC_STRLEN_DEC(s, i, 512);
	_STATIC_STRLEN_DEC(s, i, 256);
	_STATIC_STRLEN_DEC(s, i, 128);
	_STATIC_STRLEN_DEC(s, i, 64);
	_STATIC_STRLEN_DEC(s, i, 32);
	_STATIC_STRLEN_DEC(s, i, 16);
	_STATIC_STRLEN_DEC(s, i, 8);
	_STATIC_STRLEN_DEC(s, i, 4);
	_STATIC_STRLEN_DEC(s, i, 2);
	_STATIC_STRLEN_DEC(s, i, 1);
	_STATIC_STRLEN_DEC(s, i, zero);

	i >>= 1;
	BUILD_BUG_ON(i != 0 || s[i] != 0);
	return 0;

	_STATIC_STRLEN_INC(s, i, 16384);
	_STATIC_STRLEN_INC(s, i, 8192);
	_STATIC_STRLEN_INC(s, i, 4096);
	_STATIC_STRLEN_INC(s, i, 2048);
	_STATIC_STRLEN_INC(s, i, 1024);
	_STATIC_STRLEN_INC(s, i, 512);
	_STATIC_STRLEN_INC(s, i, 256);
	_STATIC_STRLEN_INC(s, i, 128);
	_STATIC_STRLEN_INC(s, i, 64);
	_STATIC_STRLEN_INC(s, i, 32);
	_STATIC_STRLEN_INC(s, i, 16);
	_STATIC_STRLEN_INC(s, i, 8);
	_STATIC_STRLEN_INC(s, i, 4);
	_STATIC_STRLEN_INC(s, i, 2);
	_STATIC_STRLEN_INC(s, i, 1);

add_zero:
	if (!s[i] && s[i - 1])
		return i;

	if (!s[0])
		return 0;

	BUILD_BUG_ON(s[0]);
	return 0;
}

#if 0
/* smaller version not using macros for illustrative purposes */
static __always_inline size_t static_strlen(const char *s) {
	size_t i = 512;

	/* if non-const or const, but longer than 4096 bytes, use rt strlen */
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
	if (is_nonzero_const(s[i + 127]))
		i += 128;
add_64:
	if (!s[i] && s[i - 1])
		return i;
	if (is_nonzero_const(s[i + 63]))
		i += 64;
add_32:
	if (!s[i] && s[i - 1])
		return i;
	if (is_nonzero_const(s[i + 31]))
		i += 32;
add_16:
	if (!s[i] && s[i - 1])
		return i;
	if (is_nonzero_const(s[i + 15]))
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
#endif

int main(int argc, char **argv) {
	size_t ret;

	ret = strlen("123456789abcdef0123456789abcde");
	return ret;
}
