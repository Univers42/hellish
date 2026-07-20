/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_lineno.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/20 16:00:00 by marvin            #+#    #+#             */
/*   Updated: 2026/07/20 16:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Depth-first search for the first token that is still a slice into the
   input buffer. The word reparser nulls a WORD node's own token and moves
   the real subtokens into its children, so the scan must recurse — the
   first in-buffer token of a simple command usually sits two levels down. */
static const char	*first_tok_start(t_ast_node *node)
{
	t_ast_node	*kids;
	const char	*found;
	size_t		i;

	if (node->token.start && !node->token.allocated)
		return (node->token.start);
	kids = (t_ast_node *)node->children.ctx;
	i = 0;
	while (i < node->children.len)
	{
		found = first_tok_start(&kids[i]);
		if (found)
			return (found);
		i++;
	}
	return (NULL);
}

/* With batched input delivery, rl.line points at the END of the batch, so
   $LINENO can no longer be read off the line counter. Instead we remember
   the executing command's first source token here (a borrowed pointer into
   alias_exp); lineno_str() lazily turns that into a line number by counting
   newlines up to its offset — only when a $LINENO expansion actually asks.
   Tokens that don't point into the cycle buffer (heap-copied function
   bodies, eval/cmdsub re-lexes) are skipped, so those keep the previous
   command's line — the call site — which is what bash reports for eval. */
void	note_cmd_lineno(t_shell *state, t_ast_node *node)
{
	const char	*start;

	start = first_tok_start(node);
	if (start)
		state->rl.ln_tok = start;
}

/* Line number of a token inside hd_stripped (the cycle buffer with heredoc
   body lines removed). Before the first heredoc, stripped text matches the
   source byte-for-byte, so the newline count is exact. Past the end of the
   first heredoc's operator line, add back every removed body line (packed
   in hd_src) — exact for the common one-heredoc cycle, an approximation
   when several heredocs interleave with commands. */
static int	hd_stripped_line(t_shell *state, const char *tok)
{
	const char	*s;
	size_t		off;
	size_t		cut;
	int			line;

	s = state->hd_stripped;
	off = tok - s;
	line = state->rl.cycle_line0 + nl_count(s, off);
	cut = 0;
	while (s[cut] && !(s[cut] == '<' && s[cut + 1] == '<'))
		cut++;
	while (s[cut] && s[cut] != '\n')
		cut++;
	if (off > cut && state->hd_src)
		line += nl_count(state->hd_src, ft_strlen(state->hd_src));
	return (line);
}

/* Resolve the executing command's line number under batched input
   delivery: rl.line points at the END of the delivered batch, so the real
   line is recovered from the command's first token offset into the cycle
   buffer (cycle start line + newlines before the token). The (ln_ptr,
   ln_val) pair memoises the scan so a loop body re-reading $LINENO costs a
   pointer compare, not a rescan. Tokens live in alias_exp normally, or in
   hd_stripped when the cycle contained heredocs; anything else (function
   bodies, eval re-lexes) keeps the last resolved line — the call site. */
int	tok_lineno(t_shell *state)
{
	const char	*base;
	const char	*tok;
	int			line;

	if (!state->rl.tok_line)
		return (state->rl.line);
	tok = state->rl.ln_tok;
	if (tok && tok == state->rl.ln_ptr)
		return (state->rl.ln_val);
	base = (const char *)state->alias_exp.ctx;
	if (tok && base && tok >= base && tok < base + state->alias_exp.len)
		line = state->rl.cycle_line0 + nl_count(base, tok - base);
	else if (tok && state->hd_stripped && tok >= state->hd_stripped
		&& tok < state->hd_stripped + ft_strlen(state->hd_stripped))
		line = hd_stripped_line(state, tok);
	else if (state->rl.ln_ptr)
		return (state->rl.ln_val);
	else
		return (state->rl.line);
	state->rl.ln_ptr = tok;
	state->rl.ln_val = line;
	return (line);
}
