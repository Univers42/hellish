/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_test_posix.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/19 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <sys/stat.h>

int	test_filecomp(const char *op, const char *f1, const char *f2);

/* POSIX-mandated argument-count disambiguation for test/[ with 2-4
   arguments, mirroring bash's two_arguments()/three_arguments() and the
   4-argument special cases of posixtest(). The point: with a fixed number
   of arguments POSIX dictates the parse by POSITION, not by precedence —
   `[ -z = -z ]` is a string comparison and `[ -z -a ] ]` is an AND of two
   non-empty-string tests, even though -z usually looks like an operator. */

/* Two arguments: `! s` negates the non-empty-string test, otherwise the
   first word must be a unary operator applied to the second. */
int	eval_two(char **av)
{
	if (ft_strcmp(av[0], "!") == 0)
		return (ft_strlen(av[1]) != 0);
	return (tx_test_unary(av));
}

/* Three arguments: a binary operator in the middle always wins; -a/-o
   AND/OR two one-argument (non-empty string) tests; `!` negates the
   two-argument rule (errors stay 2, never negated); parentheses wrap a
   one-argument test; anything else is a syntax error like bash. */
int	eval_three(char **av)
{
	int	r;

	if (tx_is_binop(av[1]))
		return (tx_test_binary(av));
	if (ft_strcmp(av[1], "-a") == 0)
		return (!(av[0][0] && av[2][0]));
	if (ft_strcmp(av[1], "-o") == 0)
		return (!(av[0][0] || av[2][0]));
	if (ft_strcmp(av[0], "!") == 0)
	{
		r = eval_two(av + 1);
		if (r == 2)
			return (2);
		return (r == 0);
	}
	if (ft_strcmp(av[0], "(") == 0 && ft_strcmp(av[2], ")") == 0)
		return (ft_strlen(av[1]) == 0);
	return (ft_eprintf("test: %s: binary operator expected\n", av[1]), 2);
}

/* Four arguments: `!` negates the three-argument rule and `( x y )` wraps
   the two-argument rule; anything else returns -1 so the caller falls
   through to the general expression grammar, exactly like bash. */
int	eval_four(char **av)
{
	int	r;

	if (ft_strcmp(av[0], "!") == 0)
	{
		r = eval_three(av + 1);
		if (r == 2)
			return (2);
		return (r == 0);
	}
	if (ft_strcmp(av[0], "(") == 0 && ft_strcmp(av[3], ")") == 0)
		return (eval_two(av + 1));
	return (-1);
}

/* The nanosecond mtime field is spelled differently per platform: POSIX
   2008 and glibc/musl call it st_mtim, Darwin calls it st_mtimespec. Same
   struct timespec either way, so the FIELD NAME is the macro and the
   comparison below reads identically on both. (The BSDs also use
   st_mtimespec; add them here if a BSD rung is ever added to CI -- they
   are not in the matrix today, so this stays a claim I have tested.) */
#ifdef __APPLE__
# define ST_MTIM st_mtimespec
#else
# define ST_MTIM st_mtim
#endif

/* Three-way mtime comparison at nanosecond resolution (bash and dash both
   compare the timespec, not just st_mtime — two files touched within the
   same second must not read as "newer"). Returns <0, 0, >0 like strcmp. */
static int	mtime_cmp(struct stat *a, struct stat *b)
{
	if (a->ST_MTIM.tv_sec != b->ST_MTIM.tv_sec)
		return ((a->ST_MTIM.tv_sec > b->ST_MTIM.tv_sec) * 2 - 1);
	if (a->ST_MTIM.tv_nsec == b->ST_MTIM.tv_nsec)
		return (0);
	return ((a->ST_MTIM.tv_nsec > b->ST_MTIM.tv_nsec) * 2 - 1);
}

/* -nt / -ot / -ef, matching bash's filecomp(): -ef is true only when both
   stats succeed and (device, inode) match; -nt / -ot rank "exists" above
   "missing" (an existing file is -nt a missing one, a missing file is -ot
   an existing one, two missing files compare false either way). */
int	test_filecomp(const char *op, const char *f1, const char *f2)
{
	struct stat	st1;
	struct stat	st2;
	int			r1;
	int			r2;

	r1 = (stat(f1, &st1) == 0);
	r2 = (stat(f2, &st2) == 0);
	if (ft_strcmp(op, "-ef") == 0)
		return (!(r1 && r2 && st1.st_dev == st2.st_dev
				&& st1.st_ino == st2.st_ino));
	if (ft_strcmp(op, "-nt") == 0)
		return (!((r1 && !r2) || (r1 && r2 && mtime_cmp(&st1, &st2) > 0)));
	return (!((!r1 && r2) || (r1 && r2 && mtime_cmp(&st1, &st2) < 0)));
}
