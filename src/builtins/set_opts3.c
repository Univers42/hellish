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

/* Set one of set's short-option letters to `on`. The caller has already
   vetted the letter against the supported set, so anything unknown is
   simply ignored here. */
static void	set_flag_char(t_shell *state, char c, bool on)
{
	if (c == 'e')
		state->opt_errexit = on;
	else if (c == 'u')
		state->opt_nounset = on;
	else if (c == 'x')
		state->opt_xtrace = on;
	else if (c == 'f')
		state->opt_noglob = on;
	else if (c == 'C')
		state->opt_noclobber = on;
	else if (c == 'a')
		state->opt_allexport = on;
	else if (c == 'n')
		state->opt_noexec = on;
	else if (c == 'v')
		state->opt_verbose = on;
}

/* Apply one flag word like "-e", "+e", "-eux". Returns false when the
   word contains a letter this shell does not implement, so the caller
   can emit bash's "invalid option" error + status 2 instead of silently
   swallowing it (a lie about what the shell honours). */
bool	apply_flag_word(t_shell *state, const char *w)
{
	char	sign;
	int		j;

	sign = w[0];
	j = 1;
	while (w[j])
	{
		if (!ft_strchr("euxfCanv", w[j]))
			return (false);
		set_flag_char(state, w[j], sign == '-');
		j++;
	}
	return (true);
}

/* Build the value of $- (the flag string): append one letter for each
   currently-enabled option. The 'i' flag appears when the shell is running
   interactively (INP_RL), not as a separately toggled option — POSIX says
   $- must include 'i' for interactive shells. We write into state->flagbuf
   (a fixed-size field in t_shell) so the returned pointer stays valid until
   the next call. env_expand("−") calls this to get the current value. */
char	*build_flagstr(t_shell *state)
{
	int	k;

	k = 0;
	if (state->opt_allexport)
		state->flagbuf[k++] = 'a';
	if (state->opt_errexit)
		state->flagbuf[k++] = 'e';
	if (state->opt_noglob)
		state->flagbuf[k++] = 'f';
	if (state->metinp == INP_RL || state->opt_interactive)
		state->flagbuf[k++] = 'i';
	if (state->opt_noexec)
		state->flagbuf[k++] = 'n';
	if (state->opt_nounset)
		state->flagbuf[k++] = 'u';
	if (state->opt_verbose)
		state->flagbuf[k++] = 'v';
	if (state->opt_xtrace)
		state->flagbuf[k++] = 'x';
	if (state->opt_noclobber)
		state->flagbuf[k++] = 'C';
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
