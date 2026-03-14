/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils6.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

bool	is_compound_start(t_tt tt)
{
	return (tt == TT_IF || tt == TT_WHILE || tt == TT_UNTIL
		|| tt == TT_FOR || tt == TT_CASE || tt == TT_LBRACE
		|| tt == TT_BANG);
}

bool	is_compound_terminator(t_tt tt)
{
	return (tt == TT_BRACE_RIGHT || tt == TT_THEN || tt == TT_ELIF
		|| tt == TT_ELSE || tt == TT_FI || tt == TT_DO
		|| tt == TT_DONE || tt == TT_ESAC || tt == TT_RBRACE);
}

bool	is_separator_before_terminator(t_ast_node *ret, t_deque_tok *tokens)
{
	size_t	len;
	t_tt	next_tt;
	t_tt	last_tt;

	len = ret->children.len;
	if (len == 0)
		return (false);
	last_tt = ((t_ast_node *)ret->children.ctx)[len - 1].token.tt;
	next_tt = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	return ((last_tt == TT_SEMICOLON || last_tt == TT_NEWLINE)
		&& is_compound_terminator(next_tt));
}

void	skip_newlines(t_deque_tok *tokens)
{
	while ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE)
		(void)deque_pop_start(&tokens->deqtok);
}
