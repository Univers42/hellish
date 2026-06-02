/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 17:12:27 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Digit for bash base#n (base 2..64).
   For base <= 36 upper and lower are interchangeable;
   above 36 a-z=10..35, A-Z=36..61, @=62 _=63. */
static int	get_digit_ext(char c, int base)
{
	int	d;

	d = -1;
	if (c >= '0' && c <= '9')
		d = c - '0';
	else if (c >= 'a' && c <= 'z')
		d = 10 + c - 'a';
	else if (c >= 'A' && c <= 'Z' && base <= 36)
		d = 10 + c - 'A';
	else if (c >= 'A' && c <= 'Z')
		d = 36 + c - 'A';
	else if (c == '@')
		d = 62;
	else if (c == '_')
		d = 63;
	if (d < 0 || d >= base)
		return (-1);
	return (d);
}

/* Parse `base#digits` (decimal base already parsed).
   Sets lex->error on a base outside 2..64,
   otherwise reads the base-N digits into *out. */
long long	parse_base_n(t_arith_lexer *lex, int *pos, long long base)
{
	long long	val;
	int			digit;

	if (base < 2 || base > 64)
		return (lex->error = true, 0);
	(*pos)++;
	val = 0;
	while (*pos < lex->len)
	{
		digit = get_digit_ext(lex->input[*pos], (int)base);
		if (digit < 0)
			break ;
		val = val * base + digit;
		(*pos)++;
	}
	return (val);
}
