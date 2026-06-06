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

/* Skip space, tab, and newline between tokens. The POSIX spec allows
   whitespace anywhere inside $((...)); bash does too, so we follow suit.
   Carriage return is intentionally NOT skipped -- matching bash behaviour
   where $'\r' inside an expression is an error. */
void	skip_whitespace(t_arith_lexer *lex)
{
	while (lex->pos < lex->len
		&& (lex->input[lex->pos] == ' '
			|| lex->input[lex->pos] == '\t'
			|| lex->input[lex->pos] == '\n'))
		lex->pos++;
}

/* A variable name can start with a letter or underscore -- the same rule as C
   identifiers and POSIX shell variable names. Used to distinguish a bare name
   like "count" from a '$' sigil form inside arithmetic expansions. */
bool	is_var_start(char c)
{
	return (ft_isalpha(c) || c == '_');
}

/* Continuation characters for a variable name: alphanumeric or underscore.
   Digits are valid here but not at the start (handled by is_var_start). */
bool	is_var_char(char c)
{
	return (ft_isalnum(c) || c == '_');
}
