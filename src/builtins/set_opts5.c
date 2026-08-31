/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_opts5.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/19 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* True for a row that is a hellish EXTENSION rather than one of bash's own
   options: settable and queryable by name, but left OUT of the `set -o` /
   `shopt -o` listing.  bash's listing is a fixed 27-line roster the golden
   suite diffs line-for-line (tests/issue8_set_options runs `set -o | wc -l`),
   so an extension that appended itself there would turn every conformance
   run red for a feature nobody asked about.  Extensions are opt-in by name
   or they do not exist.  One place to extend when there is a second one. */
bool	setopt_hidden(const t_setopt *e)
{
	return (e->bit == SETOPT_ZSH);
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
   layer; everything else is a plain flip.

   The dialect bit is NOT a plain flip, in two ways this function used to
   get wrong by flipping it directly: zsh_mode_swap mirrors the bit into
   the glob layer's cell (which stayed stale here), and an explicit
   `set -o zsh` is a global request that must outlive the file it was
   typed in -- an rc file is sourced through a call frame whose pop
   restores the dialect unconditionally, so without the pin the mode
   lasted exactly as long as the rc was being read. */
void	setopt_put(t_shell *st, const t_setopt *e, bool on)
{
	bool	*cell;

	if (e->bit == SETOPT_ZSH)
		return (zsh_mode_pin(st, on));
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
