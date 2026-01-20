/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_tokens.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:21:46 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 20:48:55 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

t_ast_node	parse_tokens(t_shell *state, t_parser *parser, t_deque_tt *tokens)
{
	t_tt		tt;
	t_ast_node	ret;
	t_token		tmp;

	parser->res = RES_OK;
	ret = parse_simple_list(state, parser, tokens);
	if (parser->res == RES_OK)
	{
		tmp = *(t_token *)deque_pop_start(&tokens->deqtok);
		tt = tmp.tt;
		if (tt != TT_END)
			ft_printf("Got token: %s\n", tt_to_str(tt));
		ft_assert(tt == TT_END);
		ft_assert(tokens->deqtok.len == 0);
		reparse_words(&ret);
		reparse_assignment_words(&ret);
	}
	if (PRINT_AST)
		print_ast_dot(state, ret);
	return (ret);
}
