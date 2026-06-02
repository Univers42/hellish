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

/* let expr [expr ...] : evaluate each arithmetic expression (assignments
   persist, like $((...))). Exit 0 if the LAST expression is non-zero, else 1. */
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
	return (v != 0 ? 0 : 1);
}
