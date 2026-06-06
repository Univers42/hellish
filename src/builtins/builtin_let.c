/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_let.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "arith.h"

/* let expr [expr ...]: evaluate each arithmetic expression in order. The
   exit status is determined only by the value of the LAST expression:
   non-zero -> 0 (true), zero -> 1 (false). This matches bash and is the
   exact inverse of what most C programmers expect. Arithmetic errors (e.g.
   invalid tokens) set err and return 1 immediately rather than continuing. */
int	builtin_let(t_shell *state, t_vec argv)
{
	char		**av;
	size_t		i;
	bool		err;
	long long	v;

	av = (char **)argv.ctx;
	if (argv.len < 2)
		return (ft_eprintf("%s: let: expression expected\n", state->ctx), 2);
	v = 0;
	i = 1;
	while (i < argv.len)
	{
		err = false;
		v = arith_eval(state, av[i], (int)ft_strlen(av[i]), &err);
		if (err)
			return (1);
		i++;
	}
	if (v != 0)
		return (0);
	return (1);
}
