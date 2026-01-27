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

#include "parser_private.h"

static int	parse_and_push_pipeline(t_shell *state,
									t_parser *parser,
									t_deque_tok *tokens,
									t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_pipeline(state, parser, tokens);
	vec_push(&ret->children, &tmp_node);
	if (parser->res != RES_OK)
		return (2);
	vec_pop(&parser->parse_stack);
	return (0);
}

static int	parse_simple_list_op(t_shell *state, t_parser *parser,
								t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt	next;
	int		r;

	r = push_simple_list_op_token(parser, tokens, ret, &next);
	if (r != 0)
		return (r);
	r = check_newlines_and_end(parser, tokens, next);
	if (r != 0)
		return (r);
	return (parse_and_push_pipeline(state, parser, tokens, ret));
}

static void	handle_final_newline_or_end(t_shell *state, t_parser *parser,
										t_ast_node *ret,
										t_deque_tok *tokens)
{
	if (is_newline_token(tokens))
		(void)deque_pop_start(&tokens->deqtok);
	else if (!is_end_token(tokens))
		handle_unexpected_token(state, parser, *ret, tokens);
}

/* Run parse_simple_list_op repeatedly until it signals "no more ops" (1)
   or "error / need more input" (2). Returns the same status code. */
static int	process_all_simple_list_ops(t_shell *state, t_parser *parser,
										t_deque_tok *tokens, t_ast_node *ret)
{
	int	status;

	while (1)
	{
		status = parse_simple_list_op(state, parser, tokens, ret);
		if (status == 1)
			return (1);
		if (status == 2)
			return (2);
	}
}

t_ast_node	parse_simple_list(t_shell *state, t_parser *parser,
								t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_tt		next;
	int			status;

	init_ast_node_children(&ret, AST_SIMPLE_LIST);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_ARITH_START)
		return (handle_arith_error(state, parser, tokens, &ret), ret);
	if (!is_simple_cmd_token(next) && next != TT_BRACE_LEFT)
		return (handle_unexpected_token(state, parser, ret, tokens), ret);
	push_parsed_pipeline_child(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	status = process_all_simple_list_ops(state, parser, tokens, &ret);
	if (status == 2)
		return (ret);
	handle_final_newline_or_end(state, parser, &ret, tokens);
	return (ret);
}
