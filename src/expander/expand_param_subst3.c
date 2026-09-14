/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_subst3.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/14 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "case_match.h"

/* "Is there a match anywhere in what is left?", asked once, before looking
   for where.
**
** ${v//pat/rep} asks patsub_match_len at every position, and that in turn
** tries every length -- n^2 whole-pattern matches to discover that a
** pattern matches nothing at all.  With an extglob group in the pattern
** each of those is itself a backtracking search, and bash-preexec's
** PROMPT_COMMAND sanitiser is exactly that shape:
**
**     ${s//?(+([[:blank:]]))[";$nl"]*([[:blank:]]):.../$nl}
**
** which normally matches nothing, and cost 7.7 seconds at 122 characters
** where bash spends none.  It is the first thing a fresh install runs.
**
** bash's match_upattern opens by wrapping the pattern in `*`s and matching
** that against the whole string: if `*pat*` does not match, no substring
** does, and the search is over.  One match instead of n^2.  The wrap is
** sound because cm_run is whole-string -- `*P*` matches exactly when P
** matches some substring -- and the answer is monotone: nothing matching in
** s means nothing matches in any suffix of s either, so it is asked again
** only after a replacement has consumed part of the value.
*/

/* Trailing backslashes in the pattern, which decide whether appending a
   `*` would land inside an escape and turn the wildcard into a literal
   asterisk. An odd count means it would; that pattern skips the shortcut
   rather than risk answering "no match" for one that does match. */
static int	patsub_tail_bs(const char *pat, int len)
{
	int	n;

	n = 0;
	while (n < len && pat[len - 1 - n] == '\\')
		n++;
	return (n);
}

int	patsub_anywhere(const char *pat, const char *s)
{
	t_string	w;
	int			len;
	int			ok;

	len = (int)ft_strlen(pat);
	if (patsub_tail_bs(pat, len) % 2)
		return (1);
	vec_init(&w);
	w.elem_size = 1;
	vec_push_char(&w, '*');
	vec_push_nstr(&w, (char *)pat, len);
	vec_push_char(&w, '*');
	vec_push_char(&w, '\0');
	ok = case_match(s, (char *)w.ctx);
	return (xfree(w.ctx), ok);
}

/* Can the pattern's first element match the character at `s` at all?
   bash's match_pattern_char, and the reason ${v//x*y/Z} does not try every
   length at every position of a value with no `x` in it: a literal head
   that disagrees with the byte in front of it cannot match here, whatever
   follows it.  A group, a `*`, a `?` and a bracket all decline to answer
   and leave the scan to run. */
int	patsub_head_ok(const char *pat, size_t plen, const char *s)
{
	if (!*s)
		return (1);
	if (xg_start(pat, pat + plen) || *pat == '*' || *pat == '?'
		|| *pat == '[')
		return (1);
	if (*pat == '\\' && plen > 1)
		return (s[0] == pat[1]);
	return (*s == *pat);
}

/* The longest match this pattern can possibly produce, in bytes, or -1
   when there is no bound.  bash's umatchlen, and what keeps the
   longest-match scan honest: ${v//[b]/X} can match one character and
   nothing longer, so trying every remaining length at every position was
   n^2 attempts to find the n that exist.
     `?` and a bracket each match one CHARACTER, so they are counted as the
   widest one a UTF-8 locale can hold -- over-counting only costs a few
   doomed attempts, under-counting would lose a match. */
int	patsub_maxlen(const char *pat, size_t plen)
{
	const char	*close;
	size_t		i;
	int			n;

	i = 0;
	n = 0;
	while (i < plen)
	{
		if (xg_start(pat + i, pat + plen) || pat[i] == '*')
			return (-1);
		close = NULL;
		if (pat[i] == '[')
			close = bracket_close(pat + i, pat + plen);
		n += 1;
		if (close || pat[i] == '?')
			n += 3;
		if (close)
			i = (size_t)(close - pat) + 1;
		else if (pat[i] == '\\' && i + 1 < plen)
			i += 2;
		else
			i++;
	}
	return (n);
}
