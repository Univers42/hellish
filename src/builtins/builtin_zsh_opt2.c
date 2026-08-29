/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_opt2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 20:05:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 20:05:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Options whose NAME starts with a literal "no" that is part of the word,
   not the inversion prefix.  Without this `setopt notify` would be read as
   "un-set tify" and `setopt nomatch` as "un-set match". */
bool	zopt_bare(const char *n)
{
	return (!ft_strcmp(n, "notify") || !ft_strcmp(n, "nomatch")
		|| !ft_strcmp(n, "noclobber") || !ft_strcmp(n, "noglob")
		|| !ft_strcmp(n, "nounset") || !ft_strcmp(n, "noexec")
		|| !ft_strcmp(n, "nolog") || !ft_strcmp(n, "nobeep"));
}

/* The shopt-backed half of the mapping: zsh's name on the left, the bit we
   already implement on the right.  Returns 0 for a name this table does not
   cover, so zopt_apply can carry on to the set -o half. */
static unsigned int	zopt_shopt_bit(const char *n)
{
	if (!ft_strcmp(n, "nullglob"))
		return (SHOPT_NULLGLOB);
	if (!ft_strcmp(n, "globdots"))
		return (SHOPT_DOTGLOB);
	if (!ft_strcmp(n, "extendedglob"))
		return (SHOPT_EXTGLOB);
	if (!ft_strcmp(n, "nocaseglob"))
		return (SHOPT_NOCASEGLOB);
	if (!ft_strcmp(n, "globstarshort"))
		return (SHOPT_GLOBSTAR);
	if (!ft_strcmp(n, "autocd"))
		return (SHOPT_AUTOCD);
	if (!ft_strcmp(n, "appendhistory"))
		return (SHOPT_HISTAPPEND);
	if (!ft_strcmp(n, "checkwinsize"))
		return (SHOPT_CHECKWINSIZE);
	return (0);
}

/* The `set -o` half: zsh's name on the left, the POSIX option it is spelled
   as here on the right.  zsh renamed several of these -- errreturn is
   errexit, printexitvalue has no analogue -- so a plain lookup in
   setopt_find() would miss them. */
static const char	*zopt_setopt_name(const char *n)
{
	if (!ft_strcmp(n, "errexit") || !ft_strcmp(n, "errreturn"))
		return ("errexit");
	if (!ft_strcmp(n, "xtrace"))
		return ("xtrace");
	if (!ft_strcmp(n, "nounset"))
		return ("nounset");
	if (!ft_strcmp(n, "noglob"))
		return ("noglob");
	if (!ft_strcmp(n, "noclobber"))
		return ("noclobber");
	if (!ft_strcmp(n, "allexport"))
		return ("allexport");
	if (!ft_strcmp(n, "pipefail"))
		return ("pipefail");
	if (!ft_strcmp(n, "monitor"))
		return ("monitor");
	if (!ft_strcmp(n, "interactivecomments"))
		return ("interactive-comments");
	if (!ft_strcmp(n, "verbose"))
		return ("verbose");
	return (NULL);
}

/* Set one option we actually implement.  True when the name was handled. */
bool	zopt_apply(t_shell *state, const char *n, bool on)
{
	const t_setopt	*e;
	unsigned int	bit;

	bit = zopt_shopt_bit(n);
	if (bit && on)
		return (state->shopt |= bit, true);
	if (bit)
		return (state->shopt &= ~bit, true);
	if (!ft_strcmp(n, "zsh"))
		return (zsh_mode_swap(state, on), true);
	e = NULL;
	if (zopt_setopt_name(n))
		e = setopt_find(zopt_setopt_name(n), 0);
	if (!e)
		return (false);
	return (setopt_put(state, e, on), true);
}

/* Real zsh options with NO behaviour here.  Accepted and ignored, because
   the alternative is worse in both directions: erroring would stop plugins
   that open with `setopt localoptions` -- most of them -- and quietly
   accepting anything at all would hide a genuine typo.  Listing them by
   name is what makes the gap documented rather than silent; anything not
   here still gets zsh's "no such option". */
bool	zopt_inert(const char *n)
{
	static const char	*t[] = {"localoptions", "localtraps",
		"warncreateglobal", "promptsubst", "promptpercent",
		"promptbang", "rcquotes", "noexec",
		"nobeep", "nomatch", "notify", "autopushd", "pushdignoredups",
		"pushdminus", "pushdsilent", "cdablevars", "extendedhistory",
		"histignoredups", "histignorespace", "histignorealldups",
		"histreduceblanks", "histverify", "incappendhistory",
		"sharehistory", "banghist", "completeinword", "alwaystoend",
		"automenu", "autolist", "listtypes", "menucomplete", "hashlistall",
		"correct", "correctall", "aliases", "multios", "shortloops",
		"typesetsilent", "kshoptionprint", "unset", "clobber", "glob",
		"caseglob", "equals", "function_argzero", "listambiguous",
		"nolisttypes", "nolog", NULL};
	int					i;

	i = -1;
	while (t[++i])
		if (!ft_strcmp(t[i], n))
			return (true);
	return (false);
}
