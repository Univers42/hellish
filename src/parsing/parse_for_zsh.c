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

/* `for x (a b) { list }` -- the brace form of the short body. */
static void	zfor_brace_body(t_shell *state, t_parser *parser,
				t_deque_tok *tokens, t_ast_node *ret)
{
	(void)deque_pop_start(&tokens->deqtok);
	push_parsed_compound_list(state, parser, tokens, ret);
	if (parser->res != RES_OK)
		return ;
	skip_newlines(tokens);
	if ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt != TT_RBRACE)
		(void)unexpected(state, parser, *ret, tokens);
	else
		(void)deque_pop_start(&tokens->deqtok);
}

/* zsh's short bodies after the paren list: `for x (a b) cmd` runs ONE
   command (a sublist, in zsh's grammar) and `for x (a b) { list }` a
   brace group -- no do/done anywhere.  oh-my-zsh.sh itself is written
   this way (`for config_file ($ZSH/lib/ *.zsh) source $config_file`),
   and so is every zsh tutorial's plugin loader.  False when the body is
   the POSIX `do ... done`, which the caller goes on to parse; true means
   the body has been consumed, or its parse failed with res set. */
static bool	zfor_short_body(t_shell *state, t_parser *parser,
				t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	body;
	t_tt		next;

	skip_newlines(tokens);
	next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_DO || next == TT_END)
		return (false);
	if (next == TT_LBRACE)
		zfor_brace_body(state, parser, tokens, ret);
	else
	{
		body = parse_body_pipeline(state, parser, tokens);
		ast_push_child(ret, &body);
	}
	if (parser->res == RES_OK)
		vec_pop(&parser->parse_stack);
	return (true);
}

/* The whole head after the first name: extra names, then either the zsh
   paren list or the POSIX `in` clause.  0 means the parse failed and the
   caller should return with parser->res already set; 1 that `do ... done`
   follows; 2 that a zsh short body was parsed and the loop is complete. */
int	for_head(t_shell *state, t_parser *parser, t_deque_tok *tokens,
			t_ast_node *ret)
{
	if (zsh_mode(state))
	{
		zfor_names(tokens, ret);
		if ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_BRACE_LEFT)
		{
			if (!zfor_paren(state, parser, tokens, ret))
				return (0);
			if (zfor_short_body(state, parser, tokens, ret))
				return (2);
			return (1);
		}
	}
	if (!for_in_clause(state, parser, tokens, ret))
		return (0);
	return (1);
}
