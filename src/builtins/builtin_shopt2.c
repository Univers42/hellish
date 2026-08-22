/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_shopt2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Split from builtin_shopt.c only because the norm caps a file at 5
   functions. */

/* Collect the flags. Every character of every leading -word is read, not
   just the first: -q is a MODIFIER that combines freely with the action
   flags, so `shopt -qs extglob` means "set it, quietly" and used to be
   parsed as a bare -q (a query) because only argv[i][1] was inspected.
   That made it report failure on an option it had just switched on.

   -o is the same shape of mistake and cost issue #51 a noisy login: it is
   a SCOPE ("read these names off the set -o roster"), not an action, so it
   gets its own out-parameter. Sharing `act` meant the last letter of a
   cluster won -- `shopt -op posix` parsed as a bare -p, which then rejected
   "posix" as an unknown shopt name -- and that `shopt -oq posix` could not
   keep both its scope and its -q. */
size_t	shopt_flags(t_vec argv, char *act, int *quiet, int *use_o)
{
	size_t	i;
	size_t	j;

	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-'
		&& ((char **)argv.ctx)[i][1])
	{
		j = 1;
		while (((char **)argv.ctx)[i][j])
		{
			if (((char **)argv.ctx)[i][j] == 'q')
				*quiet = 1;
			else if (((char **)argv.ctx)[i][j] == 'o')
				*use_o = 1;
			else if (ft_strchr("sup", ((char **)argv.ctx)[i][j]))
				*act = ((char **)argv.ctx)[i][j];
			j++;
		}
		i++;
	}
	return (i);
}

/* One name under `shopt -o`. The action rules are the same ones shopt_one
   applies to shopt's own names, against the set -o roster instead: -s and
   -u report whether the CHANGE succeeded (always 0 here, the roster has no
   read-only entries), while -q and the print forms report the SETTING, so
   `shopt -oq posix` is 1 while posix mode is off. That status is the whole
   point of the call -- Ubuntu's stock ~/.bashrc branches on it.

   bash words the rejection differently for this form ("invalid option
   name", without the "shell" the plain form uses), and the difference is
   load-bearing: it is how a script tells "no such set -o option" apart
   from "no such shopt option". */
static int	shopt_setopt_one(t_shell *state, const char *name, t_shopt_act a)
{
	const t_setopt	*e;
	bool			on;

	e = setopt_find(name, 0);
	if (!e)
		return (ft_eprintf("%s: shopt: %s: invalid option name\n",
				state->ctx, name), 1);
	if (a.act == 's' || a.act == 'u')
		return (setopt_put(state, e, a.act == 's'), 0);
	on = setopt_get(state, e);
	if (!a.quiet && a.act == 'p' && on)
		ft_printf("set -o %s\n", name);
	else if (!a.quiet && a.act == 'p')
		ft_printf("set +o %s\n", name);
	else if (!a.quiet && on)
		ft_printf("%-15s\ton\n", name);
	else if (!a.quiet)
		ft_printf("%-15s\toff\n", name);
	return (!on);
}

/* Every name after `shopt -o ...`. One bad name does not stop the rest --
   bash reports each and returns failure once, so a probe over several
   options still answers for the ones it knows. */
int	shopt_setopt(t_shell *state, t_vec argv, size_t i, t_shopt_act a)
{
	int	rc;

	rc = 0;
	while (i < argv.len)
		if (shopt_setopt_one(state, ((char **)argv.ctx)[i++], a))
			rc = 1;
	return (rc);
}
