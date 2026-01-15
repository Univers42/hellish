/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser_simple_list.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:24 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:24 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <stdbool.h>
# include "parser.h"

t_ast_node unexpected(t_shell *state, t_parser *parser, t_ast_node ret, t_deque_tt *tokens);

bool	is_simple_list_op(t_tt tt)
{
	if (tt == TT_SEMICOLON
		|| tt == TT_OR
		|| tt == TT_AND
		|| tt == TT_AMPERSAND)
		return (true);
	return (false);
}

// 0 continue
// 1 break
// 2 return on fail
int	parse_simple_list_s(t_shell *state, t_parser *parser,
	t_deque_tt *tokens, t_ast_node *ret)
{
	t_tt		next;
	t_token		tmp;

	tmp = *(t_token *)deque_peek(&tokens->deqtok);
	next = tmp.tt;
	if (!is_simple_list_op(next))
		return (1);
	{ int val = next; vec_push(&parser->parse_stack, &val); }
	tmp = *(t_token *)deque_pop_start(&tokens->deqtok);
	{
		t_ast_node tmp_node = (t_ast_node){.node_type = AST_TOKEN, .token = tmp};
		vec_init(&tmp_node.children);
		tmp_node.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp_node);
	}
	if (parser->res != RES_OK)
		return (2);
	while ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE)
		(void)deque_pop_start(&tokens->deqtok);
	/* For & and ;, if we hit END or NEWLINE, that's valid - just stop parsing */
	if ((next == TT_SEMICOLON || next == TT_AMPERSAND) 
		&& (*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_END)
		return (2);
	if ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_END)
		return (parser->res = RES_MoreInput, 2);
	{
		t_ast_node tmp_node = parse_pipeline(state, parser, tokens);
		vec_push(&ret->children, &tmp_node);
	}
	if (parser->res != RES_OK)
		return (2);
	vec_pop(&parser->parse_stack);
	return (0);
}

t_ast_node	parse_simple_list(t_shell *state,
	t_parser *parser, t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_tt		next;
	int			status;

	ret = (t_ast_node){.node_type = AST_SIMPLE_LIST};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (!is_simple_cmd_token(next) && next != TT_BRACE_LEFT)
		return (unexpected(state, parser, ret, tokens));
	{
		t_ast_node tmp_node = parse_pipeline(state, parser, tokens);
		vec_push(&ret.children, &tmp_node);
	}
	if (parser->res != RES_OK)
		return (ret);
	while (1)
	{
		status = parse_simple_list_s(state, parser, tokens, &ret);
		if (status == 1)
			break ;
		if (status == 2)
			return (ret);
	}
	if ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE)
		(void)deque_pop_start(&tokens->deqtok);
	else if ((*(t_token *)deque_peek(&tokens->deqtok)).tt != TT_END)
		return (unexpected(state, parser, ret, tokens));
	return (ret);
}
