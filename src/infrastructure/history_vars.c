/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_vars.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 16:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 16:40:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"
#include "case_match.h"

/* The HIST* variables, read where they are used rather than cached.
**
** They were not read anywhere at all: HISTCONTROL, HISTIGNORE, HISTSIZE,
** HISTFILESIZE and HISTTIMEFORMAT appeared in this tree only in a comment,
** and the dedup rule was hard-wired to bash's `ignoredups` whatever the
** user had asked for -- which is also a divergence in the other direction,
** since bash's DEFAULT HISTCONTROL is empty and keeps consecutive
** duplicates. A configuration that says
**
**     HISTCONTROL=ignoredups:ignorespace:erasedups
**     HISTIGNORE='ls:cd:exit'
**     HISTSIZE=20000
**
** got the first word of the first line by accident and nothing else: no
** leading-space suppression, no pattern list, and a hard cap of 2000.
**
** Read per use, because they are ordinary shell variables: a user may set
** them at any prompt, and a cached copy would need an invalidation hook
** that could only ever be a source of staleness. Each read is a hash
** lookup on a list of at most a few words, on a path that is already
** doing file I/O. */

/* One colon-separated word present in a variable's value? bash compares
   these case-sensitively and only as whole words: HISTCONTROL=ignoredupsx
   enables nothing. */
static bool	hist_var_has(t_shell *state, const char *var, const char *word)
{
	char	*v;
	size_t	n;

	v = env_expand(state, (char *)var);
	if (!v || !*v)
		return (false);
	n = ft_strlen(word);
	while (*v)
	{
		if (!ft_strncmp(v, word, n) && (v[n] == '\0' || v[n] == ':'))
			return (true);
		while (*v && *v != ':')
			v++;
		while (*v == ':')
			v++;
	}
	return (false);
}

/* HISTCONTROL word, honouring `ignoreboth` as the pair it abbreviates. */
bool	hist_control_has(t_shell *state, const char *word)
{
	if (hist_var_has(state, "HISTCONTROL", word))
		return (true);
	if (!ft_strcmp(word, "ignoredups") || !ft_strcmp(word, "ignorespace"))
		return (hist_var_has(state, "HISTCONTROL", "ignoreboth"));
	return (false);
}

/* HISTIGNORE: colon-separated glob patterns matched against the WHOLE
   line. `&` is bash's shorthand for "the previous history entry", which is
   what makes `HISTIGNORE='&'` a spelling of ignoredups. The patterns go
   through the shell's one matcher, so they mean here exactly what they
   mean in `case`. */
bool	hist_ignore_match(t_shell *state, const char *line)
{
	char	*v;
	char	*end;
	char	*prev;

	v = env_expand(state, "HISTIGNORE");
	if (!v || !*v || !line)
		return (false);
	while (*v)
	{
		end = v;
		while (*end && *end != ':')
			end++;
		prev = hist_last(state);
		if (end - v == 1 && *v == '&' && prev && !ft_strcmp(prev, line))
			return (true);
		if (!(end - v == 1 && *v == '&')
			&& case_match_n(line, ft_strlen(line), v, (size_t)(end - v)))
			return (true);
		v = end;
		while (*v == ':')
			v++;
	}
	return (false);
}

/* The entry this command WOULD be recorded as: the raw text, joined the
   way append_hist_entry will join it.

   The joining is the point. The dedup used to compare the raw typed text
   against the stored entry, and the stored entry is already joined -- so
   for a multi-line command the two could never be equal and no repeat of
   a `for` loop was ever recognised as a duplicate. Asking the question
   about the form that will actually be stored is the fix, and it is why
   this is shared with manage_history rather than open-coded twice. */
char	*hist_candidate(t_shell *state)
{
	char	*raw;
	char	*joined;

	if (state->input_expanded && state->input.ctx)
		raw = ft_strndup((char *)state->input.ctx, state->input.len);
	else if (state->rl.buff.ctx && state->rl.cursor > 0)
		raw = ft_strndup((char *)state->rl.buff.ctx, state->rl.cursor - 1);
	else
		return (NULL);
	if (!raw)
		return (NULL);
	joined = hist_join_line(raw, (state->shopt & SHOPT_LITHIST) != 0);
	if (!joined)
		return (raw);
	return (xfree(raw), joined);
}

/* A numeric HIST* limit. Unset is bash's default (500 for both, but the
   caller passes what it wants); empty or non-numeric is 0 in bash; a
   negative value means unlimited, which this returns as -1. */
long	hist_limit(t_shell *state, const char *var, long dflt)
{
	char	*v;
	long	n;
	int		neg;

	v = env_expand(state, (char *)var);
	if (!v)
		return (dflt);
	if (!*v)
		return (0);
	neg = (*v == '-');
	v += (neg || *v == '+');
	n = 0;
	while (*v >= '0' && *v <= '9')
		n = n * 10 + (*v++ - '0');
	if (*v)
		return (0);
	if (neg)
		return (-1);
	return (n);
}
