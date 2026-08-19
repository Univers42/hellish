/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_opts4.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/19 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* The complete `set -o` roster, in bash's own listing order (alphabetical by
   name) so list_set_options can just walk it.  `letter` is '\0' for options
   with no short form.  `bit` is the e_setopt flag holding the state, or 0
   for the ten options that own a dedicated opt_* bool in t_shell -- those
   resolve through setopt_cell() below.

   Deliberately absent: `-r` (restricted).  bash accepts it; hellish does not
   implement a restricted mode, and quietly recording a security flag we do
   not enforce is worse than failing loudly, so `set -r` stays a usage error. */
const t_setopt	*setopt_table(void)
{
	static const t_setopt	tbl[] = {
	{"allexport", 'a', 0}, {"braceexpand", 'B', SETOPT_BRACEEXPAND},
	{"emacs", 0, SETOPT_EMACS}, {"errexit", 'e', 0},
	{"errtrace", 'E', SETOPT_ERRTRACE}, {"functrace", 'T', SETOPT_FUNCTRACE},
	{"hashall", 'h', SETOPT_HASHALL}, {"histexpand", 'H', SETOPT_HISTEXPAND},
	{"history", 0, SETOPT_HISTORY}, {"ignoreeof", 0, SETOPT_IGNOREEOF},
	{"interactive-comments", 0, SETOPT_ICOMMENTS},
	{"keyword", 'k', SETOPT_KEYWORD}, {"monitor", 'm', SETOPT_MONITOR},
	{"noclobber", 'C', 0}, {"noexec", 'n', 0}, {"noglob", 'f', 0},
	{"nolog", 0, SETOPT_NOLOG}, {"notify", 'b', SETOPT_NOTIFY},
	{"nounset", 'u', 0}, {"onecmd", 't', SETOPT_ONECMD},
	{"physical", 'P', SETOPT_PHYSICAL}, {"pipefail", 0, 0},
	{"posix", 0, 0}, {"privileged", 'p', SETOPT_PRIVILEGED},
	{"verbose", 'v', 0}, {"vi", 0, SETOPT_VI}, {"xtrace", 'x', 0},
	{NULL, 0, 0}};

	return (tbl);
}

/* Address the t_shell bool backing one of the ten dedicated options.  NULL
   for anything else, which for a bit-less entry means "no storage" -- only
   possible if the table and this chain drift apart. */
static bool	*setopt_cell(t_shell *st, const char *name)
{
	if (!ft_strcmp(name, "errexit"))
		return (&st->opt_errexit);
	if (!ft_strcmp(name, "nounset"))
		return (&st->opt_nounset);
	if (!ft_strcmp(name, "xtrace"))
		return (&st->opt_xtrace);
	if (!ft_strcmp(name, "noglob"))
		return (&st->opt_noglob);
	if (!ft_strcmp(name, "noclobber"))
		return (&st->opt_noclobber);
	if (!ft_strcmp(name, "allexport"))
		return (&st->opt_allexport);
	if (!ft_strcmp(name, "noexec"))
		return (&st->opt_noexec);
	if (!ft_strcmp(name, "verbose"))
		return (&st->opt_verbose);
	if (!ft_strcmp(name, "pipefail"))
		return (&st->opt_pipefail);
	if (!ft_strcmp(name, "posix"))
		return (&st->opt_posix);
	return (NULL);
}

/* Look one option up by long name (pass NULL to search by letter instead) or
   by short letter.  Returns NULL when the option is not one bash knows. */
const t_setopt	*setopt_find(const char *name, char letter)
{
	const t_setopt	*e;

	e = setopt_table();
	while (e->name)
	{
		if (name && !ft_strcmp(e->name, name))
			return (e);
		if (!name && letter && e->letter == letter)
			return (e);
		e++;
	}
	return (NULL);
}

/* Current state of one option. */
bool	setopt_get(t_shell *st, const t_setopt *e)
{
	bool	*cell;

	if (e->bit)
		return ((st->setopt & e->bit) != 0);
	cell = setopt_cell(st, e->name);
	if (cell)
		return (*cell);
	return (false);
}

/* Turn one option on or off.  vi and emacs are mutually exclusive and also
   drive the line editor, so they get the extra hop through the readline
   layer; everything else is a plain flip. */
void	setopt_put(t_shell *st, const t_setopt *e, bool on)
{
	bool	*cell;

	if (e->bit && on)
		st->setopt |= e->bit;
	else if (e->bit)
		st->setopt &= ~e->bit;
	if (e->bit == SETOPT_EMACS || e->bit == SETOPT_VI)
		return (set_opt_edit_mode(st, e->name, on));
	cell = setopt_cell(st, e->name);
	if (cell)
		*cell = on;
}
