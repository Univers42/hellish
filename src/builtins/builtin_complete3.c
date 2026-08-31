/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_complete3.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* `complete`'s option scanner. The pointers it collects BORROW from argv;
   comp_store copies them per name, so nothing here owns memory. */

/* The value-taking options, and where each value lands. */
static int	comp_val_opt(t_vec argv, size_t i, t_cmpopt *o)
{
	char	*w;
	char	*v;

	w = ((char **)argv.ctx)[i];
	if (i + 1 >= argv.len)
		return (0);
	v = ((char **)argv.ctx)[i + 1];
	if (ft_strcmp(w, "-W") == 0)
		return (o->words = v, 1);
	if (ft_strcmp(w, "-F") == 0)
		return (o->func = v, 1);
	if (ft_strcmp(w, "-o") == 0)
		return (o->opts = v, 1);
	if (ft_strcmp(w, "-A") == 0)
	{
		o->act = cg_action_of(v);
		return (1);
	}
	if (ft_strchr("XPSCG", w[1]) && !w[2])
		return (1);
	return (0);
}

/* One option word: -p/-r are modes, -D/-E/-I are the default-spec
   selectors (accepted and ignored -- they name WHICH default to set, and
   with no default spec support there is nothing to select), the single
   action letters set act, and the rest take a value. */
static int	comp_one_opt(t_shell *st, t_vec argv, size_t i, t_cmpopt *o)
{
	char	*w;

	w = ((char **)argv.ctx)[i];
	if (ft_strcmp(w, "-p") == 0)
		return (o->print = true, 0);
	if (ft_strcmp(w, "-r") == 0)
		return (o->remove = true, 0);
	if (w[1] && !w[2] && ft_strchr("DEI", w[1]))
		return (0);
	if (w[1] && !w[2] && ft_strchr("abcdfkv", w[1]))
		return (o->act = w[1], 0);
	if (w[1] && !w[2] && ft_strchr("WFoAXPSCG", w[1]))
		return (comp_val_opt(argv, i, o));
	return (ft_eprintf("%s: complete: %s: invalid option\n", st->ctx, w), -1);
}

/* Scan leading options; returns the index of the first NAME, or
   (size_t)-1 after an error the caller reports as status 2. `--` ends the
   options here for the same reason it does in compgen: what follows is
   arbitrary text the shell must not read as a flag. */
size_t	comp_parse_opts(t_shell *st, t_vec argv, t_cmpopt *o)
{
	size_t	i;
	int		n;

	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-'
		&& ((char **)argv.ctx)[i][1])
	{
		if (ft_strcmp(((char **)argv.ctx)[i], "--") == 0)
			return (i + 1);
		n = comp_one_opt(st, argv, i, o);
		if (n < 0)
			return (CG_OPT_ERR);
		i += (size_t)n + 1;
	}
	return (i);
}

/* Session teardown for the compspec table: one entry's four strings, then
   the backing array. Called from free_all_state next to free_dirstack --
   the specs are session-lifetime state, not per-command scratch. */
void	free_compspecs(t_shell *state)
{
	size_t	i;

	i = 0;
	while (i < state->compspecs.len)
		comp_free_spec((t_compspec *)vec_idx(&state->compspecs, i++));
	xfree(state->compspecs.ctx);
	state->compspecs.ctx = NULL;
	state->compspecs.len = 0;
	state->compspecs.cap = 0;
}
