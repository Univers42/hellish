/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_bad_subst.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/14 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sh_input.h"

void	exit_clean(t_shell *state, int code);

/* bash parity for a malformed ${...}: report "bad substitution", set $?
   to 127, and abort a non-interactive shell (bash --posix -c exits 127).

   WHAT IS NAMED. bash names the word being expanded, not the ${...}
   span: `${{.ID}}` scans to the first `}` like any other expansion, so
   the span is `${{.ID}` and the trailing `}` is simply the rest of the
   word, and a message that showed the span alone hid that. Inside double
   quotes bash expands the quoted text as a word of its own, so that is
   what it names there: `${{.ID}}` for "${{.ID}}", not the quotes.

   WHAT HAPPENS NEXT. Interactively the rest of the LINE is discarded,
   which is bash's DISCARD; returning "" and carrying on ran
   `docker image ls --format }` after the error, and docker printed a `}`
   per image. */

/* Index of the span inside the word being expanded, or -1. */
static int	span_at(t_shell *state, const char *s, int slen)
{
	int	i;

	if (!state->exp_word || slen <= 0)
		return (-1);
	i = -1;
	while (++i + slen <= state->exp_wlen)
		if (!ft_memcmp(state->exp_word + i, s, (size_t)slen))
			return (i);
	return (-1);
}

/* The text to name: the double-quoted run the span sits in, when there is
   one on both sides of it, else the whole word. */
static void	named_text(t_shell *state, int at, int slen, int *out)
{
	const char	*w;
	int			lo;
	int			hi;

	w = state->exp_word;
	lo = at;
	while (--lo >= 0 && !(w[lo] == '"' && (lo == 0 || w[lo - 1] != '\\')))
		;
	hi = at + slen - 1;
	while (++hi < state->exp_wlen && !(w[hi] == '"' && w[hi - 1] != '\\'))
		;
	out[0] = 0;
	out[1] = state->exp_wlen;
	if (lo >= 0 && hi < state->exp_wlen)
	{
		out[0] = lo + 1;
		out[1] = hi - lo - 1;
	}
}

char	*pf_bad_subst(t_shell *state, const char *s, int slen)
{
	int	at;
	int	seg[2];

	at = span_at(state, s, slen);
	if (at >= 0)
	{
		named_text(state, at, slen, seg);
		ft_eprintf("%s: %.*s: bad substitution\n", state->ctx, seg[1],
			state->exp_word + seg[0]);
	}
	else
		ft_eprintf("%s: ${%.*s}: bad substitution\n", state->ctx, slen, s);
	set_cmd_status(state, create_exec_state(127, false));
	if (state->metinp != INP_RL)
		exit_clean(state, 127);
	state->discard_line = true;
	return (ft_strdup(""));
}

/* The word a bad substitution in an assignment's value is reported as: the
   value, as bash names it -- `${{a}}: bad substitution` for x=${{a}} --
   which is the text after the first '=' of the token this node carries. */
void	note_assign_value(t_shell *state, t_ast_node *src)
{
	const char	*eq;

	state->exp_word = src->token.start;
	state->exp_wlen = src->token.len;
	if (!src->token.start || src->token.len <= 0)
		return ;
	eq = ft_memchr(src->token.start, '=', (size_t)src->token.len);
	if (!eq)
		return ;
	state->exp_word = eq + 1;
	state->exp_wlen = src->token.len - (int)(eq + 1 - src->token.start);
}

/* The whole word this node carries, for the paths that flatten a word
   before expanding it: `export x=${{a}}` is reported as x=${{a}}. */
void	note_word(t_shell *state, t_ast_node *node)
{
	state->exp_word = node->token.start;
	state->exp_wlen = node->token.len;
}
