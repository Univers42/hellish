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

#include "parser_private.h"

bool	parse_compound_list_s(t_shell *state, t_parser *parser,
							t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt	op;
	t_token	tmp;

	tmp = *(t_token *)deque_peek(&tokens->deqtok);
	op = tmp.tt;
	if (!is_compund_list_op(op))
		return (true);
	tmp = *(t_token *)deque_pop_start(&tokens->deqtok);
	push_token_child(ret, tmp);
	if (is_semicolon_or_newline_before_brace_right(ret, tokens))
		return (true);
	vec_push_int(&parser->parse_stack, op);
	while ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE)
		(void)deque_pop_start(&tokens->deqtok);
	if ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_BRACE_RIGHT)
		return (true);
	if ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_END)
		return (parser->res = RES_GETMOREINPUT, true);
	{
		push_parsed_pipeline_child(state, parser, tokens, ret);
		if (parser->res != RES_OK)
			return (true);
	}
	return (vec_pop(&parser->parse_stack), false);
}

t_ast_node	parse_compound_list(t_shell *state,
								t_parser *parser, t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_tt		next;

	ret = (t_ast_node){.node_type = AST_COMPOUND_LIST};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_ARITH_START)
	{
		parser->res = RES_ERR;
		state->last_cmd_st_exe = res_status(1);
		return (ret);
	}
	push_parsed_pipeline_child(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	while (1)
	{
		if (parse_compound_list_s(state, parser, tokens, &ret))
			break ;
	}
	return (ret);
}
