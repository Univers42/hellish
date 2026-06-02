/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_case.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

static t_token	*pk(t_deque_tok *t)
{
	return ((t_token *)deque_peek(&t->deqtok));
}

static bool	is_kw_in(t_token *tk)
{
	return (tk->tt == TT_WORD && tk->len == 2
		&& ft_strncmp(tk->start, "in", 2) == 0);
}

/* Parse one clause: [(] pattern [| pattern]... ) [compound-list] [;;]
   AST_CASE_ITEM children = pattern words... + trailing compound-list body. */
static t_ast_node	parse_case_item(t_shell *state, t_parser *parser,
						t_deque_tok *tokens)
{
	t_ast_node	item;
	t_ast_node	body;

	init_ast_node_children(&item, AST_CASE_ITEM);
	if (pk(tokens)->tt == TT_BRACE_LEFT)
		(void)deque_pop_start(&tokens->deqtok);
	push_parsed_word(tokens, &item);
	while (pk(tokens)->tt == TT_PIPE)
	{
		(void)deque_pop_start(&tokens->deqtok);
		push_parsed_word(tokens, &item);
	}
	if (pk(tokens)->tt != TT_BRACE_RIGHT)
		return (parser->res = RES_ERR, item);
	(void)deque_pop_start(&tokens->deqtok);
	skip_newlines(tokens);
	if (pk(tokens)->tt == TT_DSEMI || pk(tokens)->tt == TT_ESAC)
		return (init_ast_node_children(&body, AST_COMPOUND_LIST),
			vec_push(&item.children, &body), item);
	push_parsed_compound_list(state, parser, tokens, &item);
	if (parser->res == RES_OK && pk(tokens)->tt == TT_DSEMI)
		(void)deque_pop_start(&tokens->deqtok);
	return (item);
}

/* case WORD in [pattern) list ;;]... esac */
t_ast_node	parse_case_command(t_shell *state, t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_ast_node	item;

	init_ast_node_children(&ret, AST_CASE);
	vec_push_int(&parser->parse_stack, TT_CASE);
	(void)deque_pop_start(&tokens->deqtok);
	skip_newlines(tokens);
	if (pk(tokens)->tt == TT_END)
		return (parser->res = RES_GETMOREINPUT, ret);
	push_parsed_word(tokens, &ret);
	skip_newlines(tokens);
	if (pk(tokens)->tt == TT_END)
		return (parser->res = RES_GETMOREINPUT, ret);
	if (!is_kw_in(pk(tokens)))
		return (parser->res = RES_ERR, ret);
	(void)deque_pop_start(&tokens->deqtok);
	skip_newlines(tokens);
	while (pk(tokens)->tt != TT_ESAC)
	{
		if (pk(tokens)->tt == TT_END)
			return (parser->res = RES_GETMOREINPUT, ret);
		item = parse_case_item(state, parser, tokens);
		vec_push(&ret.children, &item);
		if (parser->res != RES_OK)
			return (ret);
		skip_newlines(tokens);
	}
	return ((void)deque_pop_start(&tokens->deqtok),
		vec_pop(&parser->parse_stack), ret);
}
