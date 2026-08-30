/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_for_zsh.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 21:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 21:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* zsh's for: several variables, and a parenthesised word list.
**
**     for old_name new_name (
**       current_branch  git_current_branch
**     ); do ... done
**
** Each turn of the loop binds the names in order, so the list is consumed
** N at a time.  oh-my-zsh's git plugin ends with exactly this, and it was
** the last construct between that file and loading.
**
** The extra NAMES are stored by widening the existing name token to span
** from the first to the last -- they are adjacent in the source, so the
** span IS the text "old_name new_name" and the executor splits it.  The
** alternative, extra children, would have collided with the word list that
** already lives there and been distinguishable only by counting.
*/

/* Consume the second and later loop variables.  Stops at `in`, at `do`, at
   a separator, or at the `(` -- everything that can legally follow a name
   list.  Only reached in the zsh dialect, where `for a b` is grammatical;
   in bash it is a syntax error and stays one. */
void	zfor_names(t_deque_tok *tokens, t_ast_node *ret)
{
	t_ltoken	*peek;

	peek = (t_ltoken *)deque_peek(&tokens->deqtok);
	while (peek->tt == TT_WORD && !(peek->len == 2
			&& kw_eq(tokens->base + peek->off, "in", 2)))
	{
		ret->token.len = (int)(tokens->base + peek->off + peek->len
				- ret->token.start);
		(void)deque_pop_start(&tokens->deqtok);
		peek = (t_ltoken *)deque_peek(&tokens->deqtok);
	}
}

/* The `( word ... )` list.  Newlines inside the parens are insignificant --
   the plugin above spreads its pairs over several lines -- which is why this
   cannot reuse collect_word_list, whose job is to stop at one. */
static bool	zfor_paren(t_shell *state, t_parser *parser,
				t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt	next;

	(void)state;
	(void)deque_pop_start(&tokens->deqtok);
	skip_newlines(tokens);
	next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	while (is_for_word(next))
	{
		push_parsed_word(tokens, ret);
		skip_newlines(tokens);
		next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	}
	if (next != TT_BRACE_RIGHT)
		return (parser->res = RES_ERR, false);
	(void)deque_pop_start(&tokens->deqtok);
	ret->negate = true;
	next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_SEMICOLON || next == TT_NEWLINE)
		(void)deque_pop_start(&tokens->deqtok);
	return (true);
}

/* The whole head after the first name: extra names, then either the zsh
   paren list or the POSIX `in` clause.  False means the parse failed and
   the caller should return with parser->res already set. */
bool	for_head(t_shell *state, t_parser *parser, t_deque_tok *tokens,
			t_ast_node *ret)
{
	if (zsh_mode(state))
	{
		zfor_names(tokens, ret);
		if ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_BRACE_LEFT)
			return (zfor_paren(state, parser, tokens, ret));
	}
	return (for_in_clause(state, parser, tokens, ret));
}
