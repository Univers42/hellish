/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias_scan7.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 01:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/01 01:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sh_alias.h"

/* `case` awareness for the alias scanner -- issue #91.
**
** A case PATTERN is never a command, but the scanner used to re-arm
** command position on every `|`, `(`, `;` and newline, all of which are
** ordinary punctuation inside `case x in a|b) ... ;; c) ...`. With
** `alias ls='ls --color=auto'` active, re-sourcing any file containing
**
**     case "$1" in list|ls) ... ;; esac
**
** expanded the PATTERN `ls` and died on
**     syntax error near unexpected token `--color=auto'
** -- which is how a config framework's own loader broke the moment it was
** loaded twice. bash expands aliases in command position only; these two
** helpers teach the scan where case stops that being.
*/

/* Word-level transitions. Returns true when the word was fully accounted
   for (state advanced, command position settled); the caller then skips
   its generic keyword logic. `case` opens only where a command could
   start, so `echo case` stays an argument; the subject and the pattern
   words park command position off; `esac` closes the innermost case from
   the pattern region (an empty arm) or from body command position. */
bool	asc_case_word(t_ascan *a, const char *w, size_t len, bool plain)
{
	if (plain && len == 4 && !ft_strncmp(w, "case", 4) && a->was_cmd)
	{
		a->cs_n++;
		a->cs_st = CS_SUBJ;
		return (a->cmd_pos = false, true);
	}
	if (!a->cs_n)
		return (false);
	if (a->cs_st == CS_SUBJ)
		return (a->cs_st = CS_IN, a->cmd_pos = false, true);
	if (a->cs_st == CS_IN && plain && len == 2 && !ft_strncmp(w, "in", 2))
		return (a->cs_st = CS_PAT, a->cmd_pos = false, true);
	if (plain && len == 4 && !ft_strncmp(w, "esac", 4)
		&& (a->cs_st == CS_PAT || a->was_cmd))
	{
		a->cs_n--;
		if (a->cs_n)
			a->cs_st = CS_BODY;
		else
			a->cs_st = CS_NONE;
		return (a->cmd_pos = false, true);
	}
	if (a->cs_st == CS_PAT)
		return (a->cmd_pos = false, true);
	return (false);
}

/* `;;` and its `;&` / `;;&` fall-through cousins return the scan to the
   pattern region: what follows is a pattern and must stay unexpanded. */
static bool	asc_op_dsemi(t_ascan *a, t_ascan_src *s)
{
	char	c;

	if (s->s[s->pos] != ';'
		|| (s->s[s->pos + 1] != ';' && s->s[s->pos + 1] != '&'))
		return (false);
	while (s->s[s->pos] == ';' || s->s[s->pos] == '&')
	{
		c = s->s[s->pos];
		vec_push(&a->out, &c);
		s->pos++;
	}
	a->cs_st = CS_PAT;
	return (a->cmd_pos = false, a->wstart = true, true);
}

/* Operator-level transitions, tried before the generic operator logic.
   In the pattern region, `|` separates patterns and `(` opens one --
   neither re-arms command position -- and `)` ends the region: commands
   follow. Everything unclaimed falls through to asc_op. */
bool	asc_op_case(t_ascan *a, t_ascan_src *s)
{
	char	c;

	if (!a->cs_n)
		return (false);
	if (a->cs_st == CS_BODY)
		return (asc_op_dsemi(a, s));
	c = s->s[s->pos];
	if (a->cs_st == CS_PAT && (c == '|' || c == '('))
		return (vec_push(&a->out, &c), s->pos++, a->wstart = true, true);
	if (a->cs_st == CS_PAT && c == ')')
	{
		vec_push(&a->out, &c);
		s->pos++;
		a->cs_st = CS_BODY;
		return (a->cmd_pos = true, a->wstart = true, true);
	}
	return (false);
}
