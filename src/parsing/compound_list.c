/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser_compund_list.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:20 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:20 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <stdbool.h>
# include "parser.h"

bool	is_compund_list_op(t_tt tt)
{
	if (tt == TT_SEMICOLON
		|| tt == TT_OR
		|| tt == TT_AND
		|| tt == TT_NEWLINE
		|| tt == TT_AMPERSAND)
		return (true);
	return (false);
}

bool	parse_compound_list_s(t_shell *state, t_parser *parser,
	t_deque_tt *tokens, t_ast_node *ret)
{
	t_tt		op;
	t_token		tmp;

	tmp = *(t_token *)deque_peek(&tokens->deqtok);
	op = tmp.tt;
	if (!is_compund_list_op(op))
		return (true);
	tmp = *(t_token *)deque_pop_start(&tokens->deqtok);
	{
		t_ast_node tmp_node = (t_ast_node){.node_type = AST_TOKEN, .token = tmp};
		vec_init(&tmp_node.children);
		tmp_node.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp_node);
	}
	/* check last pushed token using generic ctx */
	if ((((t_ast_node *)ret->children.ctx)[ret->children.len - 1].token.tt == TT_SEMICOLON
			|| ((t_ast_node *)ret->children.ctx)[ret->children.len - 1].token.tt == TT_NEWLINE)
		&& (*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_BRACE_RIGHT)
		return (true);
	{ int val = op; vec_push(&parser->parse_stack, &val); }
	while ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE)
		(void)deque_pop_start(&tokens->deqtok);
	if ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_BRACE_RIGHT)
		return (true);
	if ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_END)
		return (parser->res = RES_MoreInput, true);
	{
		t_ast_node tmp_node = parse_pipeline(state, parser, tokens);
		vec_push(&ret->children, &tmp_node);
	}
	if (parser->res != RES_OK)
		return (true);
	vec_pop(&parser->parse_stack);
	return (false);
}

t_ast_node	parse_compound_list(t_shell *state,
	t_parser *parser, t_deque_tt *tokens)
{
	t_ast_node	ret;

	ret = (t_ast_node){.node_type = AST_COMPOUND_LIST};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	{
		t_ast_node tmp_node = parse_pipeline(state, parser, tokens);
		vec_push(&ret.children, &tmp_node);
	}
	if (parser->res != RES_OK)
		return (ret);
	while (1)
	{
		if (parse_compound_list_s(state, parser, tokens, &ret))
			break ;
	}
	return (ret);
}
