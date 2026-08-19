/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cmdsub_fast2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Eligibility scan for the forkless $(...) path (cmdsub_fast.c).  The rule
   set errs on the side of forking: anything not provably side-effect-free
   is rejected.  Quoted spans hide metacharacters; a nested $( ) is fine
   (its own capture isolates it — recursively via this same fast path); a
   ${...} is fine unless it contains '=' or '?' -- the ${v=w}/${v:=w}
   assignment forms would leak into the parent, and ${v?w}/${v:?w} is
   FATAL: running it in-process took the whole shell down, where bash only
   kills the substitution's child and carries on with an empty result;
   $((...)) is rejected outright since arithmetic can assign; every
   control/redirect operator forks. */

/* The command word must be one of the builtins that cannot touch shell
   state and cannot legitimately fail the whole shell. */
static bool	csf_word(const char *s, int len)
{
	static const char	*ok[] = {"echo", "pwd", "true", ":", "printf", NULL};
	int					i;

	i = 0;
	while (ok[i])
	{
		if ((int)ft_strlen(ok[i]) == len && !ft_strncmp(ok[i], s, len))
			return (true);
		i++;
	}
	return (false);
}

/* Skip a balanced nested $( ... ) span; returns the index after the
   closing paren or -1 when unbalanced (reject). */
int	csf_skip_csub(const char *s, int i)
{
	int	depth;

	depth = 1;
	i += 2;
	while (s[i] && depth > 0)
	{
		if (s[i] == '(')
			depth++;
		else if (s[i] == ')')
			depth--;
		i++;
	}
	if (depth != 0)
		return (-1);
	return (i);
}

/* Scan one ${...}: reject the assignment forms ('=') and the error form
   ('?'), which exits the shell it runs in. */
int	csf_skip_param(const char *s, int i)
{
	i += 2;
	while (s[i] && s[i] != '}')
	{
		if (s[i] == '=' || s[i] == '?')
			return (-1);
		i++;
	}
	if (!s[i])
		return (-1);
	return (i + 1);
}

static int	csf_scan_char(const char *s, int i)
{
	if (s[i] == '\'' || s[i] == '"')
	{
		i = csf_skip_quoted(s, i);
		return (i);
	}
	if (s[i] == '$' && s[i + 1] == '(' && s[i + 2] == '(')
		return (-1);
	if (s[i] == '$' && s[i + 1] == '(')
		return (csf_skip_csub(s, i));
	if (s[i] == '$' && s[i + 1] == '{')
		return (csf_skip_param(s, i));
	if (ft_strchr("|&;<>`()\n", s[i]))
		return (-1);
	return (i + 1);
}

/* Full eligibility: shell modes that could abort mid-expansion fork; the
   first word must be whitelisted; then every byte must scan clean. */
bool	csf_eligible(t_shell *state, const char *s)
{
	int	i;
	int	w;

	if (state->opt_errexit || state->opt_nounset || state->opt_noexec)
		return (false);
	if (!alias_table_empty(&state->aliases))
		return (false);
	i = 0;
	while (s[i] == ' ' || s[i] == '\t')
		i++;
	w = i;
	while (s[w] && !ft_strchr(" \t\n", s[w]) && !ft_strchr("|&;<>`()'\"$",
			s[w]))
		w++;
	if (!csf_word(s + i, w - i))
		return (false);
	i = w;
	while (s[i])
	{
		i = csf_scan_char(s, i);
		if (i < 0)
			return (false);
	}
	return (true);
}
