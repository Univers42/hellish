/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_ansic2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* Single-character $'...' escapes as a pair table: odd positions are the
   escape letters, even positions the decoded bytes. Returns -1 for a
   character that is not a simple escape (numeric forms, \c, unknown). */
int	ansic_simple(char c)
{
	const char	*map;
	int			i;

	map = "n\nt\tr\ra\ab\bf\fv\013e\033E\033\\\\''\"\"??";
	i = 0;
	while (map[i])
	{
		if (map[i] == c)
			return ((unsigned char)map[i + 1]);
		i += 2;
	}
	return (-1);
}

/* One digit of a numeric escape: octal for kind 'o', hex otherwise.
   Returns the digit value or -1 to end the digit run. */
int	ansic_digit(char c, char kind)
{
	if (kind == 'o')
	{
		if (c >= '0' && c <= '7')
			return (c - '0');
		return (-1);
	}
	if (c >= '0' && c <= '9')
		return (c - '0');
	if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	if (c >= 'A' && c <= 'F')
		return (c - 'A' + 10);
	return (-1);
}

/* UTF-8 encode a \u / \U code point into the decode buffer. */
void	ansic_utf8(t_ansic *a, long cp)
{
	if (cp < 0x80)
		a->dst[a->n++] = (char)cp;
	else if (cp < 0x800)
	{
		a->dst[a->n++] = (char)(0xC0 | (cp >> 6));
		a->dst[a->n++] = (char)(0x80 | (cp & 63));
	}
	else if (cp < 0x10000)
	{
		a->dst[a->n++] = (char)(0xE0 | (cp >> 12));
		a->dst[a->n++] = (char)(0x80 | ((cp >> 6) & 63));
		a->dst[a->n++] = (char)(0x80 | (cp & 63));
	}
	else
	{
		a->dst[a->n++] = (char)(0xF0 | (cp >> 18));
		a->dst[a->n++] = (char)(0x80 | ((cp >> 12) & 63));
		a->dst[a->n++] = (char)(0x80 | ((cp >> 6) & 63));
		a->dst[a->n++] = (char)(0x80 | (cp & 63));
	}
}

/* Numeric escapes: \xHH (up to 2 hex), \nnn (up to 3 octal, the first
   digit already at i+1), \uHHHH / \UHHHHHHHH (UTF-8 encoded). The digit
   run may stop early at any non-digit, exactly like bash. */
void	ansic_num(t_ansic *a, char kind)
{
	long	v;
	int		left;
	int		d;

	a->i += 2;
	if (kind == 'o')
		a->i -= 1;
	left = 2 + (kind == 'o') + 2 * (kind == 'u') + 6 * (kind == 'U');
	v = 0;
	while (left > 0 && a->i < a->len)
	{
		d = ansic_digit(a->s[a->i], kind);
		if (d < 0)
			break ;
		if (kind == 'o')
			v = v * 8 + d;
		else
			v = v * 16 + d;
		a->i++;
		left--;
	}
	if (kind == 'u' || kind == 'U')
		return (ansic_utf8(a, v));
	a->dst[a->n++] = (char)v;
}
