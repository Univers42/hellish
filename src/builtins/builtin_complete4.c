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
   restores specs round-trips through exactly this text.
   Each option gets its own -o, which is what makes it re-readable: the
   whole list after one -o ("-o filenames dirnames") re-read as `-o
   filenames` plus a command named `dirnames`, so a save/restore pass
   silently registered a spec for the wrong name and dropped an option. */
void	comp_print_one(t_compspec *c)
{
	int	i;

	ft_printf("complete");
	i = -1;
	while (++i < 9)
		if (pc_opt_has(c->opts, co_name_at(i)))
			ft_printf(" -o %s", co_name_at(i));
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
