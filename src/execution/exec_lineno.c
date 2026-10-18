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
   the text it was lexed from), and where_follow moves the error context
   there: a batch's line from the cycle buffer, a sourced file's from its
   chunk. Tokens that point into neither (heap-copied function bodies,
   eval/cmdsub re-lexes) are skipped, so those keep the previous command's
   line -- the call site -- which is what bash reports for eval. */
void	note_cmd_lineno(t_shell *state, t_ast_node *node)
{
	const char	*start;

	start = first_tok_start(node);
	if (!start)
		return ;
	state->rl.ln_tok = start;
	where_follow(state, start);
}

/* The text this cycle's tokens slice, when it holds `tok`: alias_exp
   normally, hd_stripped when the cycle held heredocs -- whose bodies left
   an empty line apiece behind, so both count lines as the source does. */
static const char	*cycle_text(t_shell *state, const char *tok)
{
	const char	*base;

	base = (const char *)state->alias_exp.ctx;
	if (base && tok >= base && tok < base + state->alias_exp.len)
		return (base);
	base = state->hd_stripped;
	if (base && tok >= base && tok < base + state->hd_stripped_len)
		return (base);
	return (NULL);
}

/* The line of `tok` in this cycle's text, or -1 when it is not there.
   The (ln_ptr, ln_val) pair memoises the last answer, and a token further
   on (or back) in the same text resolves from it, so a batch run top to
   bottom is scanned once and a loop body re-reading $LINENO costs a scan
   of that body, not of everything before it. */
int	cycle_lineno(t_shell *state, const char *tok)
{
	const char	*base;
	const char	*m;

	m = state->rl.ln_ptr;
	if (m && tok == m)
		return (state->rl.ln_val);
	base = cycle_text(state, tok);
	if (!base)
		return (-1);
	if (m && cycle_text(state, m) == base)
		state->rl.ln_val += nl_between(m, tok);
	else
		state->rl.ln_val = state->rl.cycle_line0 + nl_count(base,
				(size_t)(tok - base));
	state->rl.ln_ptr = tok;
	return (state->rl.ln_val);
}

/* $LINENO: the executing command's line -- in the sourced file when it
   comes from one, else in the batch (the reader's counter at a prompt,
   whose line counter keeps its per-entry semantics). Anything else --
   function bodies, eval re-lexes -- keeps the last resolved line: the
   call site. */
int	tok_lineno(t_shell *state)
{
	const char	*tok;
	int			line;

	tok = state->rl.ln_tok;
	if (tok && srcpos_has(&state->err_pos, tok))
		return (srcpos_line(&state->err_pos, state->err_line, tok));
	if (!state->rl.tok_line)
		return (state->rl.line);
	line = -1;
	if (tok)
		line = cycle_lineno(state, tok);
	if (line >= 0)
		return (line);
	if (state->rl.ln_ptr)
		return (state->rl.ln_val);
	return (state->rl.line);
}
