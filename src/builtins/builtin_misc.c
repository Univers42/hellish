/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_misc.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <sys/stat.h>

/* true: always succeeds. Registered in the dispatch table so it runs as a
   builtin instead of forking /bin/true — saves a fork/exec in loops. */
int	builtin_true(t_shell *state, t_vec argv)
{
	(void)state;
	(void)argv;
	return (0);
}

/* false: always fails (returns 1). Same deal as true — avoids a fork. */
int	builtin_false(t_shell *state, t_vec argv)
{
	(void)state;
	(void)argv;
	return (1);
}

/* Validate and parse a bare octal string (no 0 prefix required). Returns -1
   on the first non-octal character so we can give a clean error message
   instead of silently truncating, and rejects values above 07777 the way
   bash's read_octal does (the per-digit check also prevents overflow). */
static int	parse_octal(const char *s, mode_t *out)
{
	mode_t	m;

	m = 0;
	if (!*s)
		return (-1);
	while (*s)
	{
		if (*s < '0' || *s > '7')
			return (-1);
		m = m * 8 + (*s - '0');
		if (m > 07777)
			return (-1);
		s++;
	}
	*out = m;
	return (0);
}

/* Route the mode operand: a leading digit means octal (bash checks only
   the first character), anything else goes through the symbolic parser,
   which works on the allowed bits (~mask).  Errors leave *m untouched so
   the caller never commits a half-parsed mask. */
static int	umask_newmask(t_shell *state, const char *arg, mode_t *m)
{
	int	allowed;

	if (arg[0] >= '0' && arg[0] <= '9')
	{
		if (parse_octal(arg, m) == 0)
			return (0);
		return (ft_eprintf("%s: umask: %s: octal number out of range\n",
				state->ctx, arg), 1);
	}
	allowed = umask_sym_parse(arg, (~(*m)) & 0777);
	if (allowed < 0)
		return (ft_eprintf("%s: umask: %s: invalid symbolic mode\n",
				state->ctx, arg), 1);
	*m = (~allowed) & 0777;
	return (0);
}

/* umask [-Sp] [mode]: print or set the file-creation mask.  Only the
   first operand counts -- bash silently ignores extras in both octal and
   symbolic form ("umask 013 077" sets 013).  With -S and a mode, the new
   mask is printed back symbolically after being set, like bash. */
int	builtin_umask(t_shell *state, t_vec argv)
{
	char	**av;
	int		idx;
	int		flags;
	mode_t	m;

	av = (char **)argv.ctx;
	idx = 1;
	flags = umask_opts(av, argv.len, &idx);
	if (flags < 0)
		return (ft_eprintf("%s: umask: invalid option\n", state->ctx), 2);
	m = umask(0);
	umask(m);
	if ((size_t)idx >= argv.len)
		return (umask_report(m, flags));
	if (umask_newmask(state, av[idx], &m) != 0)
		return (1);
	umask(m);
	if (flags & 1)
		return (umask_symbolic(flags));
	return (0);
}
