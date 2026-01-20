/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers1.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 13:18:52 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 13:34:25 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

void	skip_whitespace(t_arith_lexer *lex)
{
	while (lex->pos < lex->len
		&& (lex->input[lex->pos] == ' '
			|| lex->input[lex->pos] == '\t'
			|| lex->input[lex->pos] == '\n'))
		lex->pos++;
}

bool	is_var_start(char c)
{
	return (ft_isalpha(c) || c == '_');
}

bool	is_var_char(char c)
{
	return (ft_isalnum(c) || c == '_');
}
