/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_compopt3.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "libft.h"

/* compopt [-o|+o option] [-DEI] [name ...]
**
** With NAMEs it edits those specs for good; with none it edits the spec of
** the completion function running right now, for this TAB only.
**
** -D/-E/-I select the default, empty-line and initial-word specs. hellish
** parses `complete -D` and stores nothing for it (builtin_complete.c), so
** there is no such spec to edit and the honest answer is bash's own for a
** name it does not know -- which is also what bash prints when no default
** spec has been registered. */
int	builtin_compopt(t_shell *state, t_vec argv)
{
	size_t	i;
	int		rc;

	rc = co_scan(state, argv, &i);
	if (rc)
		return (rc);
	if (i < argv.len)
		return (co_named(state, argv, i));
	if (co_defsel(argv))
		return (ft_eprintf("%s: compopt: _DefaultCmD_: no completion "
				"specification\n", state->ctx), 1);
	return (co_current(state, argv));
}

/* A usage error: what was wrong with WORD, then bash's usage line. The
   usage line carries no shell prefix in bash, and the caller's status is
   always 2. */
int	co_usage(t_shell *st, const char *w, const char *why)
{
	ft_eprintf("%s: compopt: %s: %s\ncompopt: usage: compopt "
		"[-o|+o option] [-DEI] [name ...]\n", st->ctx, w, why);
	return (2);
}

/* Was a -D/-E/-I selector given? */
bool	co_defsel(t_vec argv)
{
	size_t	i;
	char	*w;

	i = 0;
	while (++i < argv.len)
	{
		w = ((char **)argv.ctx)[i];
		if (w[0] == '-' && w[1] && !w[2] && ft_strchr("DEI", w[1]))
			return (true);
	}
	return (false);
}
