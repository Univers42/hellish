/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   printf_quote2.c                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "printf_private.h"

/* The ANSI-C half of %q: the $'...' form, one byte at a time. */

/* One byte inside $'...'. Named escapes where bash uses one (note \E, not
   \e -- bash emits the capital), a three-digit octal for every other
   unprintable, and the byte itself otherwise. */
void	pq_ansi_char(t_string *out, unsigned char c)
{
	static const char	from[] = "\a\b\f\n\r\t\v\033\\'";
	static const char	to[] = "abfnrtvE\\'";
	char				*p;

	p = ft_strchr(from, c);
	if (c && p)
	{
		vec_push_char(out, '\\');
		vec_push_char(out, to[p - from]);
		return ;
	}
	if (c >= 32 && c != 127)
		return ((void)vec_push_char(out, (char)c));
	vec_push_char(out, '\\');
	vec_push_char(out, (char)('0' + ((c >> 6) & 7)));
	vec_push_char(out, (char)('0' + ((c >> 3) & 7)));
	vec_push_char(out, (char)('0' + (c & 7)));
}

/* The whole-string $'...' form. */
void	pq_ansi(t_string *out, const char *s)
{
	vec_push_str(out, "$'");
	while (*s)
		pq_ansi_char(out, (unsigned char)*s++);
	vec_push_char(out, '\'');
}
