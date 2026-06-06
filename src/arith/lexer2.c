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

/* Digit value for bash's base#N notation (bases 2..64). The encoding mirrors
   bash exactly: 0-9 → 0..9, a-z → 10..35. For base ≤ 36 A-Z is an alias
   for a-z (case-insensitive). For base > 36 A-Z → 36..61, @ → 62, _ → 63.
   Returns -1 if the character is invalid for this base, which stops the
   digit-consuming loop in parse_base_n. */
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

/* Parse the `base#digits` form after the decimal base has already been read
   by lex_number. The '#' itself is consumed here (++pos). Bases outside 2..64
   trigger lex->error and return 0 immediately -- bash does the same. The rest
   is a straightforward digit accumulation using get_digit_ext. */
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
