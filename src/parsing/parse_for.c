/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_for.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

static bool	expect_for_kw(t_shell *state, t_parser *parser,
						t_deque_tok *tokens, t_tt expected)
{
	t_tt	next;

	(void)state;
	skip_newlines(tokens);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_END)
	{
		parser->res = RES_GETMOREINPUT;
		return (false);
	}
	if (next != expected)
	{
		parser->res = RES_ERR;
		return (false);
	}
	(void)deque_pop_start(&tokens->deqtok);
	return (true);
}

static bool	is_word_token(t_tt tt)
{
	return (tt == TT_WORD || tt == TT_SQWORD || tt == TT_DQWORD
		|| tt == TT_ENVVAR || tt == TT_DQENVVAR);
}

static void	collect_word_list(t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt	next;

	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	while (is_word_token(next))
	{
		push_parsed_word(tokens, ret);
		next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	}
}

static bool	parse_for_in_clause(t_shell *state, t_parser *parser,
								t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt	next;
	t_token	*peek;

	(void)state;
	skip_newlines(tokens);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_END)
		return (parser->res = RES_GETMOREINPUT, false);
	peek = (t_token *)deque_peek(&tokens->deqtok);
	if (next == TT_WORD && peek->len == 2
		&& ft_strncmp(peek->start, "in", 2) == 0)
	{
		ret->negate = true;
		(void)deque_pop_start(&tokens->deqtok);
		collect_word_list(tokens, ret);
		next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
		if (next == TT_SEMICOLON || next == TT_NEWLINE)
			(void)deque_pop_start(&tokens->deqtok);
	}
	else if (next == TT_SEMICOLON || next == TT_NEWLINE)
		(void)deque_pop_start(&tokens->deqtok);
	return (true);
}

/*
** for NAME [in wordlist ;] do compound_list done
** AST_FOR: token=NAME, children=[word_list...] + compound_list(body)
*/
t_ast_node	parse_for_command(t_shell *state, t_parser *parser,
							t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_token		name_tok;

	init_ast_node_children(&ret, AST_FOR);
	vec_push_int(&parser->parse_stack, TT_FOR);
	(void)deque_pop_start(&tokens->deqtok);
	name_tok = *(t_token *)deque_pop_start(&tokens->deqtok);
	ret.token = name_tok;
	if (!parse_for_in_clause(state, parser, tokens, &ret))
		return (ret);
	if (!expect_for_kw(state, parser, tokens, TT_DO))
		return (ret);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	if (!expect_for_kw(state, parser, tokens, TT_DONE))
		return (ret);
	vec_pop(&parser->parse_stack);
	return (ret);
}
