/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_bracket.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/15 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"
#include <stdlib.h>
#include <string.h>

/*
** Expand a character range like a-z into individual characters
** Returns number of characters added to buf
*/
static int	expand_range(char start, char end, char *buf, int buf_pos)
{
	char	c;
	int		count;

	count = 0;
	if (start > end)
	{
		c = start;
		start = end;
		end = c;
	}
	c = start;
	while (c <= end)
	{
		buf[buf_pos + count] = c;
		count++;
		c++;
	}
	return (count);
}

/*
** Check for POSIX character class [:name:]
** Input: s points to the '[' of [:name:] within the bracket content
** Returns length consumed (including [: and :]), or 0 if not valid
*/
static int	check_posix_class(const char *s, int len, char *buf, int *buf_pos)
{
	static const struct {
		const char	*pattern;  /* includes leading [: */
		int			plen;
		const char	*chars;
	} classes[] = {
		{"[:alnum:]", 9, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"},
		{"[:alpha:]", 9, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"},
		{"[:digit:]", 9, "0123456789"},
		{"[:lower:]", 9, "abcdefghijklmnopqrstuvwxyz"},
		{"[:upper:]", 9, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
		{"[:space:]", 9, " \t\n\r\f\v"},
		{"[:blank:]", 9, " \t"},
		{"[:punct:]", 9, "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"},
		{"[:xdigit:]", 10, "0123456789ABCDEFabcdef"},
		{"[:cntrl:]", 9, "\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037\177"},
		{"[:graph:]", 9, "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"},
		{"[:print:]", 9, " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"},
		{NULL, 0, NULL}
	};
	int	i;

	/* Must start with [: */
	if (len < 2 || s[0] != '[' || s[1] != ':')
		return (0);
	i = 0;
	while (classes[i].pattern)
	{
		if (len >= classes[i].plen &&
			ft_strncmp(s, classes[i].pattern, classes[i].plen) == 0)
		{
			size_t chars_len = ft_strlen(classes[i].chars);
			ft_memcpy(buf + *buf_pos, classes[i].chars, chars_len);
			*buf_pos += chars_len;
			return (classes[i].plen);
		}
		i++;
	}
	return (0);
}

/*
** Expand bracket expression content into a character set
** Input: content between outer [ and ] (e.g., for [[:alpha:]], input is "[:alpha:]")
** Returns: malloc'd string containing all matched characters
*/
char	*glob_expand_bracket(const char *start, int len, int *out_len)
{
	char	*buf;
	int		buf_pos;
	int		i;
	int		consumed;

	if (len <= 0)
	{
		*out_len = 0;
		return (NULL);
	}
	buf = malloc(512);
	if (!buf)
		return (NULL);
	buf_pos = 0;
	i = 0;
	while (i < len && buf_pos < 500)
	{
		/* Check for POSIX character class [:name:] */
		if (i + 1 < len && start[i] == '[' && start[i + 1] == ':')
		{
			consumed = check_posix_class(start + i, len - i, buf, &buf_pos);
			if (consumed > 0)
			{
				i += consumed;
				continue ;
			}
		}
		/* Handle - at start or end as literal */
		if (start[i] == '-' && (i == 0 || i == len - 1))
		{
			buf[buf_pos++] = '-';
			i++;
			continue ;
		}
		/* Check for range like a-z (but not at start/end where - is literal) */
		if (i + 2 < len && start[i + 1] == '-' && start[i + 2] != ']'
			&& start[i + 2] != '[')
		{
			buf_pos += expand_range(start[i], start[i + 2], buf, buf_pos);
			i += 3;
			continue ;
		}
		/* Single character (skip if part of unrecognized [:...:] sequence) */
		buf[buf_pos++] = start[i];
		i++;
	}
	buf[buf_pos] = '\0';
	*out_len = buf_pos;
	return (buf);
}

/*
** Check if character matches a bracket expression
*/
bool	glob_char_in_class(char c, t_glob *bracket)
{
	int		i;
	bool	found;

	if (!bracket->char_set || bracket->char_set_len == 0)
		return (false);
	found = false;
	i = 0;
	while (i < bracket->char_set_len)
	{
		if (bracket->char_set[i] == c)
		{
			found = true;
			break ;
		}
		i++;
	}
	if (bracket->flags & BRACKET_NEGATED)
		return (!found);
	return (found);
}
