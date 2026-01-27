/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_pipeline.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:22:03 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

static int	process_pipeline_pipes(t_shell *state, t_parser *parser,
								t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	while ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_PIPE)
	{
		(void)deque_pop_start(&tokens->deqtok);
		while ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE)
			(void)deque_pop_start(&tokens->deqtok);
		if ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_END)
		{
			parser->res = RES_MoreInput;
			return (1);
		}
		tmp_node = parse_command(state, parser, tokens);
		vec_push(&ret->children, &tmp_node);
		if (parser->res != RES_OK)
			return (2);
	}
	return (0);
}

t_ast_node	parse_pipeline(t_shell *state, t_parser *parser, t_deque_tok *tokens)
{
	t_ast_node	ret;
	int			r;

	ret = create_node_type(AST_COMMAND_PIPELINE);
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	push_cmd_parsed(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	vec_push_int(&parser->parse_stack, TT_PIPE);
	r = process_pipeline_pipes(state, parser, tokens, &ret);
	vec_pop(&parser->parse_stack);
	if (r == 1 || r == 2)
		return (ret);
	return (ret);
}
