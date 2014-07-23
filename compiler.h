/*
 * compiler.h -- assisting macros for exploiting compiler capabilities
 * Copyright (C) 2014 Daniel Santos <daniel.santos@pobox.com> and others
 * Largely copied from Linux Kernel tree
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#ifndef _COMPILER_H_
#define _COMPILER_H_

#include <inttypes.h>

/* a few helpful gcc-extension macros */
#ifndef __aligned
# define __aligned(n)		__attribute__((aligned(n)))
#endif

#ifndef assume_aligned
# define assume_aligned(p, ...) __builtin_assume_aligned(p, ## __VA_ARGS__)
#endif

#ifndef __always_inline
# define __always_inline	inline __attribute__((always_inline))
#endif

#ifndef __noinline
# define __noinline		__attribute__((noinline))
#endif

#ifndef __flatten
# define __flatten		__attribute__((flatten))
#endif

#ifndef __compiletime_error
# define __compiletime_error(msg) __attribute__((error(msg)))
#endif

#ifndef __compiletime_warning
# define __compiletime_warning(msg) __attribute__((warning(msg)))
#endif

#define __compiletime_assert(condition, msg, prefix, suffix)		\
	do {								\
		int __cond = !(condition);				\
		extern void prefix ## suffix(void) __compiletime_error(msg); \
		if (__cond)						\
			prefix ## suffix();				\
	} while (0)

#define _compiletime_assert(condition, msg, prefix, suffix) \
	__compiletime_assert(condition, msg, prefix, suffix)

/**
 * compiletime_assert - break build and emit msg if condition is false
 * @condition: a compile-time constant condition to check
 * @msg:       a message to emit if condition is false
 *
 * In tradition of POSIX assert, this macro will break the build if the
 * supplied condition is *false*, emitting the supplied error message if the
 * compiler has support to do so.
 */
#define compiletime_assert(condition, msg) \
	_compiletime_assert(condition, msg, __compiletime_assert_, __LINE__)




/* Force a compilation error if a constant expression is not a power of 2 */
#define BUILD_BUG_ON_NOT_POWER_OF_2(n)			\
	BUILD_BUG_ON((n) == 0 || (((n) & ((n) - 1)) != 0))

/* Force a compilation error if condition is true, but also produce a
   result (of value 0 and type size_t), so the expression can be used
   e.g. in a structure initializer (or where-ever else comma expressions
   aren't permitted). */
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define BUILD_BUG_ON_NULL(e) ((void *)sizeof(struct { int:-!!(e); }))

/*
 * BUILD_BUG_ON_INVALID() permits the compiler to check the validity of the
 * expression but avoids the generation of any code, even if that expression
 * has side-effects.
 */
#define BUILD_BUG_ON_INVALID(e) ((void)(sizeof((__force long)(e))))

/**
 * BUILD_BUG_ON_MSG - break compile if a condition is true & emit supplied
 *		      error message.
 * @condition: the condition which the compiler should know is false.
 *
 * See BUILD_BUG_ON for description.
 */
#ifndef __OPTIMIZE__
#define BUILD_BUG_ON_MSG(cond, msg) do {} while(0)
#else
#define BUILD_BUG_ON_MSG(cond, msg) compiletime_assert(!(cond), msg)
#endif

/**
 * BUILD_BUG_ON - break compile if a condition is true.
 * @condition: the condition which the compiler should know is false.
 *
 * If you have some code which relies on certain constants being equal, or
 * some other compile-time-evaluated condition, you should use BUILD_BUG_ON to
 * detect if someone changes it.
 *
 * The implementation uses gcc's reluctance to create a negative array, but gcc
 * (as of 4.4) only emits that error for obvious cases (e.g. not arguments to
 * inline functions).  Luckily, in 4.3 they added the "error" function
 * attribute just for this type of case.  Thus, we use a negative sized array
 * (should always create an error on gcc versions older than 4.4) and then call
 * an undefined function with the error attribute (should always create an
 * error on gcc 4.3 and later).  If for some reason, neither creates a
 * compile-time error, we'll still have a link-time error, which is harder to
 * track down.
 */
#ifndef __OPTIMIZE__
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#else
#define BUILD_BUG_ON(condition) \
	BUILD_BUG_ON_MSG(condition, "BUILD_BUG_ON failed: " #condition)
#endif

/**
 * BUILD_BUG - break compile if used.
 *
 * If you have some code that you expect the compiler to eliminate at
 * build time, you should use BUILD_BUG to detect if it is
 * unexpectedly used.
 */
#define BUILD_BUG() BUILD_BUG_ON_MSG(1, "BUILD_BUG failed")

#define assert_const(expr) BUILD_BUG_ON_MSG(				\
	!__builtin_constant_p(expr), "Not a constant expression: " #expr)

#define assert_early(expr)						\
	do {								\
		if (__builtin_constant_p(expr))				\
			BUILD_BUG_ON_MSG(!(expr), "!(" #expr ")");	\
		else							\
			assert(expr);					\
	} while (0)

#define __static_warn_msg(condition, msg, prefix, suffix)		\
	do {								\
		int __cond = 1 || !(condition);				\
		void __noinline __compiletime_warning(msg)	\
		prefix ## suffix(void) {__asm("");}			\
		if (__cond)						\
			prefix ## suffix();				\
	} while (0)

#define _static_warn_msg(condition, msg, prefix, suffix) \
	__static_warn_msg(condition, msg, prefix, suffix)
#define static_warn_msg(condition, msg) \
	_static_warn_msg(condition, msg, __static_warn_, __LINE__)


static __always_inline /*__attribute__((alloc_align(2))) */void *
aligned_ptr(void *p, size_t align) {
	return p;
	//return (void*)((uintptr_t)p & (~((uintptr_t)0) & (align - 1)));
}

#define aligned_alloca(n, align) (void *)				\
	({								\
		const size_t a = align - 1;				\
		return (void *) (((uintptr_t)alloca((n) + a) + a) & ~a);\
	})

#endif /* _COMPILER_H_ */
