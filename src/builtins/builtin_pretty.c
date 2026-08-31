/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_pretty.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* `pretty` -- named presets for the behaviour knobs that change how the
   shell FEELS, as opposed to what it computes.

   Why this exists (issue #32). The multi-line history recall people want is
   one `shopt` bit, `lithist`. That is fine if you already know the name.
   Nobody does: `shopt` lists eleven options with no indication of which
   ones are cosmetic, which are POSIX-affecting, and which one is the thing
   you actually wanted. The answer "run shopt -s lithist" was given on #32
   and the reporter came back saying the problem continued -- a setting you
   have to be told about twice is not discoverable.

   So `pretty` is a curated FRONT-END over those same shopt bits. No new
   state: every feature below is one SHOPT_* flag, which means `shopt` and
   `pretty` can never disagree, and everything already built on those bits
   keeps working untouched.

   Reproducibility is the point of `pretty -p`: it prints the exact lines
   to paste into ~/.hellishrc, so a configuration can be copied between
   machines instead of remembered.

     pretty                 what is on right now
     pretty -p              the same, as ~/.hellishrc lines
     pretty list            every feature and mode, with descriptions
     pretty on  NAME...     turn features on
     pretty off NAME...     turn features off
     pretty mode NAME       apply a preset (plain | friendly | full)
*/

/* Feature table: the curated name, the shopt bit it IS, and one line of
   what it does. Order is the order `pretty list` prints. */
t_pret	*pretty_table(void)
{
	static t_pret	tab[] = {
	{"multiline-history", SHOPT_LITHIST,
		"recall keeps a compound's newlines and indentation"},
	{"cd-spell", SHOPT_CDSPELL,
		"cd fixes a small typo in a directory name"},
	{"auto-cd", SHOPT_AUTOCD,
		"a bare directory name changes to it"},
	{"resize-aware", SHOPT_CHECKWINSIZE,
		"track the terminal size after every command"},
	{"deep-glob", SHOPT_GLOBSTAR,
		"** matches across directory levels"},
	{"extended-glob", SHOPT_EXTGLOB,
		"?() *() +() @() !() pattern operators"},
	{"case-blind-glob", SHOPT_NOCASEGLOB,
		"globs ignore case"},
	{"hidden-glob", SHOPT_DOTGLOB,
		"* also matches names starting with a dot"},
	{NULL, 0, NULL}};

	return (tab);
}

/* Find one feature by its curated name, or NULL. */
static t_pret	*pretty_find(const char *name)
{
	t_pret	*t;

	t = pretty_table();
	while (t->name)
	{
		if (!ft_strcmp(t->name, name))
			return (t);
		t++;
	}
	return (NULL);
}

/* `pretty on|off NAME...` -- flip each named feature. An unknown name is
   an error and stops nothing else from having been applied, which matches
   how shopt treats a bad name in a list. */
static int	pretty_set(t_shell *state, t_vec argv, int first, bool on)
{
	t_pret	*t;
	char	**av;
	int		st;

	av = (char **)argv.ctx;
	st = 0;
	if (first >= (int)argv.len)
		return (ft_eprintf("%s: pretty: needs at least one feature name"
				" (try `pretty list`)\n", state->ctx), 2);
	while (first < (int)argv.len)
	{
		t = pretty_find(av[first]);
		if (!t)
			st = (ft_eprintf("%s: pretty: %s: unknown feature (try"
						" `pretty list`)\n", state->ctx, av[first]), 2);
		else if (on)
			state->shopt |= t->bit;
		else
			state->shopt &= ~t->bit;
		first++;
	}
	return (glob_opts_sync(state), st);
}

/* pretty [-p] | list | on ... | off ... | mode NAME */
int	builtin_pretty(t_shell *state, t_vec argv)
{
	char	**av;

	av = (char **)argv.ctx;
	if (argv.len < 2)
		return (pretty_show(state, false));
	if (!ft_strcmp(av[1], "-p"))
		return (pretty_show(state, true));
	if (!ft_strcmp(av[1], "list"))
		return (pretty_list(state));
	if (!ft_strcmp(av[1], "on"))
		return (pretty_set(state, argv, 2, true));
	if (!ft_strcmp(av[1], "off"))
		return (pretty_set(state, argv, 2, false));
	if (!ft_strcmp(av[1], "mode"))
		return (pretty_mode(state, argv, 2));
	return (ft_eprintf("%s: pretty: %s: expected -p, list, on, off or"
			" mode\n", state->ctx, av[1]), 2);
}
