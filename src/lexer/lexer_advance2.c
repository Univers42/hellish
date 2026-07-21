/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer_advance2.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:55 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:55 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

/* Advance across a $'...' ANSI-C quoted region, *str on the '$'. Unlike a
   plain single-quoted string, backslash escapes are live here — \' does
   NOT terminate the region ($'it\'s' is one string). Returns 1 when the
   input ends before the closing quote (caller asks for more input). */
int	advance_ansic(char **str)
{
	*str += 2;
	while (**str && **str != '\'')
	{
		if (**str == '\\' && (*str)[1])
			(*str)++;
		(*str)++;
	}
	if (**str != '\'')
		return (1);
	(*str)++;
	return (0);
}

/* Skip a backslash-escaped character. If str[1] exists we step over both
   the backslash and the next char; otherwise we step over just the backslash.
   The second case (trailing backslash) shouldn't arise in valid input, but
   this is defensive against truncated strings. */
void	advance_bs(char **str)
{
	ft_assert(**str == '\\');
	if ((*str)[1])
		*str += 1;
	*str += 1;
}
