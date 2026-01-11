/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 15:30:24 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/06 20:27:40 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ERROR_H
#define ERROR_H

#include <setjmp.h>
#include <signal.h>

typedef struct s_errg
{
	int suppressint;
	volatile sig_atomic_t intpending;
} t_errg;

/**
 * barrier()
 * purpose:
 * 	prevent the compiler from reordering memory accesses across this point
 * 	This is a "compiler memory barrier" implemented with GCC inline asm.
 *
 * @note Detail:
 * the inline asm template is emtpy("") so no machine instruction is necesarily
 * emitted.
 * - The "memory" clobber tells the compiler thta the assembly may read or
 * write any memory location. The compiler therefore must assume memory has changed
 *  and will not move loads/stores across this asm block.
 * - Because the asm is marked volatile (via __volatile__),
 * the compiler also will not remove the `asm` as dead code.
 *
 * @note Context:
 * Using the asm inline prevent from reordering or optimizing memory
 * operations across the barrier, useful for coordinating with signal
 * handlers or preventing  register caching or shared state.
 *
 * @note Warning:
 * This is not a full CPU memory fence on all architectures: it doesn't
 * necessarily emit harware fence instruction like `DSB`for correct
 * multi-core synchronization we still need proper atomic operations
 * or explicti hardware memory barriers.
 *
 * @example:
 * ```c
 * void foo(void)
 * {
 * 		x = 1;			// writes
 * 		barrier();		// prevent compiler reordering across this point
 * 		r = y;			// do reads that must not be moved before the writes
 * }
 * ```
 * This code give back an error in compilation because this is part
 * of C11
 */
static inline void barrier(void)
{
	__asm__ __volatile__("" : : : "memory");
}

static inline t_errg *get_global_err(void)
{
	static t_errg e = {0, 0};

	return (&e);
}

static inline void intoff(void)
{
	t_errg *err;

	err = get_global_err();
	err->suppressint++;
	barrier();
}

static inline void onint(void);

static inline void inton(void)
{
	t_errg *e;

	e = get_global_err();
	barrier();
	e->suppressint--;
	if (e->suppressint == 0 && e->intpending)
		onint();
}

static inline void forceinton(void)
{
	t_errg *e;

	e = get_global_err();
	barrier();
	e->suppressint = 0;
	if (e->intpending)
		onint();
}

static inline int saveint(void)
{
	return (get_global_err()->suppressint);
}

static inline void restoreint(int v)
{
	t_errg *e;

	e = get_global_err();
	barrier();
	e->suppressint = v;
	if (e->suppressint == 0 && e->intpending)
		onint();
}

static inline void clear_pending_int(void)
{
	t_errg *e;

	e = get_global_err();
	e->intpending = 0;
}

static inline sig_atomic_t int_pending(void)
{
	return (get_global_err()->intpending);
}

static inline void onint(void)
{
}

#endif
