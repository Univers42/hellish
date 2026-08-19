/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_opts3.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "sh_input.h"

/* Apply one of set's short-option letters.  The roster in set_opts4.c is the
   single source of truth for which letters exist, so this is a lookup rather
   than a chain that has to be kept in sync by hand.  false = no such letter. */
static bool	set_flag_char(t_shell *state, char c, bool on)
{
	const t_setopt	*e;

	e = setopt_find(NULL, c);
	if (!e)
		return (false);
	setopt_put(state, e, on);
	return (true);
}

/* Apply the letters of one flag word like "-e", "+e", "-eux" or "-euo".
   Letters take effect left to right and an unknown one stops the scan with
   false, so the caller can emit bash's "invalid option" error + status 2 --
   the ones already applied stay applied, which is what bash does too.

   `o` is not a flag of its own: it asks for a long option NAME, which lives
   in the next argument word.  We only record that here (*want_o) because
   this function cannot see the rest of argv; set_flag_word() consumes it.
   That indirection is the whole reason `set -euo pipefail` used to abort. */
bool	apply_flag_letters(t_shell *state, const char *w, bool *want_o)
{
	int	j;

	j = 1;
	while (w[j])
	{
		if (w[j] == 'o')
			*want_o = true;
		else if (!set_flag_char(state, w[j], w[0] == '-'))
			return (false);
		j++;
	}
	return (true);
}

/* Build the value of $- : one letter per enabled option, in bash's own order
   (lowercase alphabetical, then uppercase alphabetical), then the invocation
   letter -- 'c' for -c, 's' for a piped stdin, nothing for a script file.
   'i' is not a settable option: it reports interactivity, so it has no table
   entry and is tested directly.  We write into state->flagbuf (a fixed field
   in t_shell) so the returned pointer stays valid until the next call;
   env_expand("-") calls this to get the current value. */
char	*build_flagstr(t_shell *state)
{
	static const char	ord[] = "abefhikmnptuvxBCEHPT";
	const t_setopt		*e;
	int					i;
	int					k;

	i = -1;
	k = 0;
	while (ord[++i])
	{
		e = setopt_find(NULL, ord[i]);
		if (e && setopt_get(state, e))
			state->flagbuf[k++] = ord[i];
		else if (ord[i] == 'i' && (state->metinp == INP_RL
				|| state->opt_interactive))
			state->flagbuf[k++] = 'i';
	}
	if (state->metinp == INP_ARG)
		state->flagbuf[k++] = 'c';
	else if (state->metinp == INP_NOTTY)
		state->flagbuf[k++] = 's';
	state->flagbuf[k] = '\0';
	return (state->flagbuf);
}

/* Like pos_build but the strings are BORROWED (pointer copy, no strdup): used
   for a function call's $1.. which live in the caller's argv for the call's
   duration. shift/set promote to an owned copy (pos_build) before mutating, so
   the borrowed strings are never freed by pos_free. */
void	pos_borrow(t_pos *pos, char **args, size_t n)
{
	size_t	i;

	pos->args = ft_calloc(n + 1, sizeof(char *));
	i = 0;
	while (pos->args && args && i < n)
	{
		pos->args[i] = args[i];
		i++;
	}
	pos->count = (int)n;
	pos->args_owned = false;
	pos_set_cnt(pos);
}
