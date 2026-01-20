/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 14:18:26 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

static int	get_base(const char *input, int pos, int len, int *new_pos)
{
	if (input[pos] == '0' && pos + 1 < len)
	{
		if (input[pos + 1] == 'x' || input[pos + 1] == 'X')
		{
			*new_pos = pos + 2;
			return (16);
		}
		else if (ft_isdigit(input[pos + 1]))
		{
			*new_pos = pos + 1;
			return (8);
		}
	}
	*new_pos = pos;
	return (10);
}

static int	get_digit(char c, int base)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	if (base == 16 && c >= 'a' && c <= 'f')
		return (10 + c - 'a');
	if (base == 16 && c >= 'A' && c <= 'F')
		return (10 + c - 'A');
	return (-1);
}

static long long	parse_digits(const char *input, int *pos, int len, int base)
{
	long long	val;
	int			digit;

	val = 0;
	while (*pos < len)
	{
		digit = get_digit(input[*pos], base);
		if (digit < 0 || digit >= base)
			break ;
		val = val * base + digit;
		(*pos)++;
	}
	return (val);
}

void	lex_number(t_arith_lexer *lex)
{
	int			start;
	int			base;
	long long	val;
	int			pos;

	start = lex->pos;
	base = get_base(lex->input, lex->pos, lex->len, &pos);
	val = parse_digits(lex->input, &pos, lex->len, base);
	lex->current.type = ATOK_NUM;
	lex->current.num_val = val;
	lex->current.start = lex->input + start;
	lex->current.len = pos - start;
	lex->pos = pos;
}

void	arith_lexer_advance(t_arith_lexer *lex)
{
	char	c;

	skip_whitespace(lex);
	if (lex->pos >= lex->len)
	{
		lex->current.type = ATOK_EOF;
		lex->current.start = lex->input + lex->pos;
		lex->current.len = 0;
		return ;
	}
	c = lex->input[lex->pos];
	if (ft_isdigit(c))
		lex_number(lex);
	else if (is_var_start(c))
		lex_variable(lex);
	else
		lex_operator(lex);
}
