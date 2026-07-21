/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparser.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:35 by marvin            #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* These four helpers used to round-trip through a t_reparser: copy the
   104-byte current node in, push into the copy, copy it back out — twice
   per subtoken, ~26MB of struct traffic on a 50k-line parse. They now
   operate directly on `ret`; the t_reparser dance remains only in the
   dquote/envvar families whose helpers genuinely share threaded state. */

/* Consume a single-quoted region and emit a TT_SQWORD subtoken covering
   the content (not the surrounding quotes). Single-quoting in POSIX is
   "preserve everything literally" -- no expansions, no escapes, not even
   a backslash. The ft_assert guards verify the caller placed *i on a quote
   and that the region ended properly (no runaway string). */
void	reparse_squote(t_ast_node *ret, int *i, t_token t)
{
	int	start;

	ft_assert(t.start[(*i)++] == '\'');
	start = *i;
	while (*i < t.len && t.start[*i] != '\'')
		(*i)++;
	push_subtoken_node(ret, t, create_interval(start, *i), TT_SQWORD);
	ft_assert(t.start[(*i)++] == '\'');
}

/* Consume one backslash escape outside quotes and emit a TT_SQWORD subtoken
   (the escaped char is treated as literal, same as single-quoting). The edge
   case is a backslash at end-of-token (start--): the lexer may have
   produced a trailing backslash for a continuation line; we still push it as
   a literal rather than dropping it. */
void	reparse_bs(t_ast_node *ret, int *i, t_token t)
{
	int	start;

	ft_assert(t.start[(*i)++] == '\\');
	start = *i;
	if (*i == t.len)
		start--;
	else
		(*i)++;
	push_subtoken_node(ret, t, create_interval(start, *i), TT_SQWORD);
}

/* Consume a run of plain characters (no quotes, $, \, or backtick) and emit
   a TT_WORD subtoken. The loop stops at any "special" character so the main
   dispatch loop (loop_node_rp) can handle it. The fallthrough (*i)++ after
   the loop is the safety valve: if we ended up on a lone special we couldn't
   classify, advance one character anyway to prevent an infinite loop. */
void	reparse_norm_word(t_ast_node *ret, int *i, t_token t)
{
	int	start;

	start = *i;
	while (*i < t.len && !is_special_char(t.start[*i])
		&& t.start[*i] != '\\' && t.start[*i] != '`')
		(*i)++;
	if (*i == start && *i < t.len)
		(*i)++;
	push_subtoken_node(ret, t, create_interval(start, *i), TT_WORD);
}

/* Consume a `...` backtick command substitution and emit the whole span as
   a single TT_WORD subtoken -- process_word_token handles the actual $(...)
   expansion later. We scan for the closing backtick, honouring backslash
   escapes inside (POSIX: \\ \$ \` are the only active escapes in backtick
   context). split_eligible is true here because the output of an unquoted
   command substitution is subject to IFS word splitting. */
void	reparse_backtick(t_ast_node *ret, int *i, t_token t)
{
	int	start;

	start = (*i)++;
	while (*i < t.len && t.start[*i] != '`')
	{
		if (t.start[*i] == '\\' && *i + 1 < t.len)
			(*i)++;
		(*i)++;
	}
	*i += (*i < t.len);
	push_subtoken_node(ret, t, create_interval(start, *i), TT_WORD);
	((t_ast_node *)ret->children.ctx)[ret->children.len - 1]
		.token.split_eligible = true;
}

/* Dispatch loop: walk the token character by character and call the right
   sub-parser for each quoting/expansion context. Spaces inside a plain
   (unquoted) word are emitted as TT_WORD subtokens -- the expander later
   uses them as IFS split boundaries. Unknown characters fall through to
   reparse_norm_word, which absorbs them as a literal run. */
void	loop_node_rp(t_reparser *rp)
{
	while (rp->i < rp->current_token.len)
	{
		if (rp->current_token.start[rp->i] == '"')
			reparse_dquote(&rp->current_node, &rp->i, rp->current_token);
		else if (rp->current_token.start[rp->i] == '\\')
			reparse_bs(&rp->current_node, &rp->i, rp->current_token);
		else if (rp->current_token.start[rp->i] == '\'')
			reparse_squote(&rp->current_node, &rp->i, rp->current_token);
		else if (rp->current_token.start[rp->i] == '$')
			reparse_envvar(&rp->current_node, &rp->i,
				rp->current_token, TT_ENVVAR);
		else if (rp->current_token.start[rp->i] == '`')
			reparse_backtick(&rp->current_node, &rp->i, rp->current_token);
		else if (is_space(rp->current_token.start[rp->i]))
		{
			push_subtoken_node(&rp->current_node, rp->current_token,
				create_interval(rp->i, rp->i + 1), TT_WORD);
			rp->i++;
		}
		else
			reparse_norm_word(&rp->current_node, &rp->i, rp->current_token);
	}
}
