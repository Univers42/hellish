/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_multi_line2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/20 16:00:00 by marvin            #+#    #+#             */
/*   Updated: 2026/07/20 16:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* Batched line delivery for non-interactive input. Feeding the lexer one
   line at a time re-tokenizes and re-parses the accumulated construct on
   every appended line — O(k²) for a k-line function body, which made script
   parsing ~7x slower than bash. Since the whole source is already sitting
   in the ring buffer (INP_FILE/INP_ARG preload it, INP_NOTTY reads 8K
   blocks), we can hand over every complete line at once and lex/parse the
   batch a single time. The only constructs that LEGITIMATELY need
   line-at-a-time reading are the ones that can change how LATER lines are
   lexed or consumed: alias/unalias definitions and sourced files (alias
   splicing happens before the lexer), heredocs (bodies must not be parsed
   as commands), and backslash-newline (line accounting). hazard_at() spots
   those conservatively — a false positive only means we fall back to the
   old, always-correct single-line path for that stretch. */

/* Every byte of the input passes through here, so the order of the tests is
   load-bearing, not stylistic. Dispatching on s[i] FIRST turns the two
   keyword probes from unconditional libc strncmp calls into a byte compare
   that fails on ~95% of input: a strncmp against "alias" can only succeed
   where s[i] is already 'a', so the gate is exact, not heuristic. Before it,
   scanning a 2MB script cost ~4M strncmp calls -- 17% of every instruction
   retired during a parse, spent proving that 'x' is not the start of
   "source". */
static bool	hazard_at(const char *s, size_t i, size_t n)
{
	const char	c = s[i];

	if (c == '\\' && i + 1 < n && s[i + 1] == '\n')
		return (true);
	if (c == '<' && i + 1 < n && s[i + 1] == '<')
		return (true);
	if (c == 'a' && n - i >= 5 && ft_strncmp(s + i, "alias", 5) == 0)
		return (true);
	if (c == 's' && n - i >= 6 && ft_strncmp(s + i, "source", 6) == 0)
		return (true);
	if (c == '.' && (i + 1 == n || s[i + 1] == ' ' || s[i + 1] == '\t')
		&& (i == 0 || ft_strchr(" \t\n;&|", s[i - 1]) != NULL))
		return (true);
	return (false);
}

/* Longest deliverable span from the cursor: clip at the first hazard, then
   retreat to the last complete line ('\n' included). 0 means "no full safe
   line here" and the caller falls back to single-line delivery. */
static size_t	batch_span(t_rl *l)
{
	const char	*s;
	size_t		n;
	size_t		clip;

	s = (const char *)l->buff.ctx + l->cursor;
	n = l->buff.len - l->cursor;
	clip = 0;
	while (clip < n && !hazard_at(s, clip, n))
		clip++;
	while (clip > 0 && s[clip - 1] != '\n')
		clip--;
	return (clip);
}

int	nl_count(const char *s, size_t n)
{
	size_t	i;
	int		count;

	i = 0;
	count = 0;
	while (i < n)
	{
		if (s[i] == '\n')
			count++;
		i++;
	}
	return (count);
}

/* Per-cycle bookkeeping, run when the input accumulator is still empty:
   remember which source line this cycle starts on and whether $LINENO may
   be derived from token offsets (never for the interactive REPL, whose
   line counter must keep its per-entry semantics). */
void	begin_cycle(t_shell *state, t_string *ret)
{
	if (ret->len != 0 || state->rl.line_exact)
		return ;
	state->rl.cycle_line0 = state->rl.line + 1;
	state->rl.tok_line = (state->metinp != INP_RL);
	state->rl.batched = false;
	state->rl.ln_tok = NULL;
	state->rl.ln_ptr = NULL;
}

/* Deliver every complete hazard-free line in one go. Only the FIRST
   delivery of a cycle may batch: continuation rounds go line-by-line so a
   command executed mid-construct (an alias definition pulled in whole by
   round one) can never leak later lines into its own cycle. A cursor
   below exact_until means a failed batched cycle was rewound for exact
   replay — keep serving single lines until past the failure point. Line
   accounting must match per-line delivery: bump rl.line once per newline
   and refresh the "script: line N" error context once. Returns 4 (data
   delivered) or 0 (caller must use the single-line path). */
int	return_batch(t_shell *state, t_string *ret)
{
	t_rl	*l;
	size_t	span;

	l = &state->rl;
	if (!l->has_line || !l->buff.ctx || l->line_exact || ret->len != 0
		|| l->cursor < l->exact_until)
		return (0);
	span = batch_span(l);
	if (span == 0)
		return (0);
	l->batched = true;
	l->line += nl_count((const char *)l->buff.ctx + l->cursor, span);
	update_ctx(state);
	vec_push_nstr(ret, (char *)l->buff.ctx + l->cursor, span);
	l->cursor += span;
	l->has_line = (l->cursor != l->buff.len);
	return (4);
}
