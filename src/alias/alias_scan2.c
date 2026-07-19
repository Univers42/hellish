/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias_scan2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/19 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sh_alias.h"

/* True while the given name is an active expansion source: POSIX forbids
   re-expanding a name whose own value is still being spliced, which is
   what turns `alias e='echo e'; e` into `echo e` instead of a loop. */
bool	asc_active(t_ascan *a, const char *w, size_t len)
{
	int	i;

	i = 1;
	while (i < a->depth)
	{
		if (a->src[i].name && ft_strlen(a->src[i].name) == len
			&& !ft_strncmp(a->src[i].name, w, len))
			return (true);
		i++;
	}
	return (false);
}

/* Measure the word starting at p.  A word runs until an unquoted blank,
   newline or operator character; quoted spans, backslash pairs, $(...)
   and `...` bodies belong to the word.  *plain reports whether the word
   is a bare literal (no quoting, no expansions) — only those may be
   alias names per POSIX. */
static size_t	asc_word_span(const char *p, bool *plain)
{
	size_t	i;

	i = 0;
	*plain = true;
	while (p[i] && !ft_strchr(" \t\n;|&()<>", p[i]))
	{
		if (p[i] == '\'' || p[i] == '"')
			i += asc_span_quote(p + i);
		else if (p[i] == '\\' && p[i + 1])
			i += 2;
		else if (p[i] == '$')
			i += asc_span_dollar(p + i);
		else if (p[i] == '`')
			i += asc_span_btick(p + i);
		else
		{
			i++;
			continue ;
		}
		*plain = false;
	}
	return (i);
}

/* Try to expand w as an alias: eligible words are plain literals in
   command position (or right after a blank-ended expansion).  On success
   the value becomes the new scan source and the word itself is dropped. */
static bool	asc_try_alias(t_ascan *a, const char *w, size_t len)
{
	char	buf[256];
	char	*val;

	if (len == 0 || len >= sizeof(buf))
		return (false);
	ft_memcpy(buf, w, len);
	buf[len] = '\0';
	val = alias_get(a->aliases, buf);
	if (!val)
		return (false);
	if (asc_active(a, w, len))
		return (false);
	return (asc_push(a, w, len, val));
}

/* After a word is emitted verbatim, update the command-position state:
   reserved words like `then` keep the next word in command position,
   `for`/`case`/`in` take it out, an assignment prefix keeps it, and any
   ordinary word ends it until the next separator. */
static void	asc_word_state(t_ascan *a, const char *w, size_t len, bool plain)
{
	if (a->after_redir)
		a->after_redir = false;
	else if (plain && asc_kw_cmdnext(w, len))
		a->cmd_pos = true;
	else if (plain && asc_kw_noncmd(w, len))
		a->cmd_pos = false;
	else if (asc_is_assign(w, len))
		;
	else
		a->cmd_pos = false;
	a->chk_next = false;
	a->wstart = false;
}

/* Handle one word at the cursor: here-doc delimiters and redirection
   targets are copied through untouched; an eligible alias word is
   replaced by pushing its value as the new source; everything else is
   emitted and folded into the position-tracking state machine.  A pure
   digit run glued to < or > is an fd prefix, not a word. */
void	asc_word(t_ascan *a)
{
	t_ascan_src	*s;
	size_t		len;
	bool		plain;

	s = &a->src[a->depth - 1];
	len = asc_word_span(s->s + s->pos, &plain);
	if (a->pend_hd)
	{
		asc_hd_delim(a, s->s + s->pos, len);
		vec_push_nstr(&a->out, s->s + s->pos, len);
		s->pos += len;
		return ;
	}
	if (asc_digits_redir(s->s + s->pos, len))
	{
		vec_push_nstr(&a->out, s->s + s->pos, len);
		s->pos += len;
		return ;
	}
	if (!a->after_redir && (a->cmd_pos || a->chk_next) && plain
		&& !asc_is_assign(s->s + s->pos, len)
		&& asc_try_alias(a, s->s + s->pos, len))
	{
		s->pos += len;
		a->chk_next = false;
		a->wstart = true;
		return ;
	}
	vec_push_nstr(&a->out, s->s + s->pos, len);
	asc_word_state(a, s->s + s->pos, len, plain);
	s->pos += len;
}
