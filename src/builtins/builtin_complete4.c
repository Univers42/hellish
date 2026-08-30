/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_complete4.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Printing specs back in the form that would recreate them. */

/* One spec as the `complete` command that would recreate it. bash prints
   the options in a fixed order and single-quotes the -W list, and the
   output has to be re-readable: a completion script that saves and
   restores specs round-trips through exactly this text. */
void	comp_print_one(t_compspec *c)
{
	ft_printf("complete");
	if (c->opts)
		ft_printf(" -o %s", c->opts);
	if (c->act)
		ft_printf(" -%c", c->act);
	if (c->func)
		ft_printf(" -F %s", c->func);
	if (c->words)
		ft_printf(" -W '%s'", c->words);
	ft_printf(" %s\n", c->name);
}

void	comp_print_all(t_shell *st)
{
	size_t	i;

	i = 0;
	while (i < st->compspecs.len)
		comp_print_one((t_compspec *)vec_idx(&st->compspecs, i++));
}
