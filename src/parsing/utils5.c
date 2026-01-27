/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils5.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 21:11:01 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

void	handle_arith_error_print(t_shell *state,
							bool has_inner_paren,
							t_token last_word)
{
	ft_eprintf("%s: ((: ", state->context);
	if (has_inner_paren && last_word.start)
		ft_eprintf("missing `)' (error token is \"%.*s)\")\n",
			last_word.len, last_word.start);
	else if (last_word.start)
		ft_eprintf("syntax error in expression (error token is \"%.*s\")\n",
			last_word.len, last_word.start);
	else
		ft_eprintf("syntax error in arithmetic expression\n");
}

int	push_simple_list_op_token(t_parser *parser,
									t_deque_tok *tokens,
									t_ast_node *ret,
									t_tt *out_next)
{
	t_token	tmp;
	t_tt	next;

	tmp = *(t_token *)deque_peek(&tokens->deqtok);
	next = tmp.tt;
	if (!is_simple_list_op(next))
		return (1);
	vec_push_int(&parser->parse_stack, next);
	tmp = *(t_token *)deque_pop_start(&tokens->deqtok);
	push_token_child(ret, tmp);
	if (parser->res != RES_OK)
		return (2);
	*out_next = next;
	return (0);
}

int	check_newlines_and_end(t_parser *parser,
									t_deque_tok *tokens,
									t_tt next)
{
	while (is_newline_token(tokens))
		(void)deque_pop_start(&tokens->deqtok);
	if ((next == TT_SEMICOLON || next == TT_AMPERSAND) && is_end_token(tokens))
		return (2);
	if (is_end_token(tokens))
	{
		parser->res = RES_MoreInput;
		return (2);
	}
	return (0);
}

void	add_op_token_child(t_ast_node *ret, t_token op_tok)
{
	t_ast_node	op_node;

	op_node = create_node_tok(AST_TOKEN, op_tok);
	vec_init(&op_node.children);
	op_node.children.elem_size = sizeof(t_ast_node);
	vec_push(&ret->children, &op_node);
}

int	proc_sub_handle_eof(t_parser *parser,
							t_deque_tok *tokens,
							t_string *cmd_str,
							t_token curr)
{
	if (curr.tt == TT_END)
	{
		parser->res = RES_MoreInput;
		tokens->looking_for = ')';
		free(cmd_str->ctx);
		return (1);
	}
	return (0);
}
