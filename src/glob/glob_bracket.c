/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_bracket.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 10:58:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/*
** Expand a character range like a-z into individual characters
** Returns number of characters added to buf
*/
static int expand_range(char start, char end, char *buf, int buf_pos)
{
	char c;
	int count;

	count = 0;
	if (start > end)
		return (-1); // signal invalid range
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
static int check_posix_class(const char *s, int len, char *buf, int *buf_pos)
{
	int i;
	size_t chars_len;

	if (len < 2 || s[0] != '[' || s[1] != ':')
		return (0);
	i = 0;
	while (classes[i].pattern)
	{
		if (len >= classes[i].plen &&
			ft_strncmp(s, classes[i].pattern, classes[i].plen) == 0)
		{
			chars_len = ft_strlen(classes[i].chars);
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
char *glob_expand_bracket(const char *start, int len, int *out_len)
{
	char *buf;
	int buf_pos;
	int i;
	int consumed;
	int range_count;

	if (len <= 0)
	{
		*out_len = 0;
		return (NULL);
	}
	// POSIX: []a] is equivalent to [a], i.e. just 'a'
	if (len >= 2 && start[0] == ']' && len == 2)
	{
		buf = malloc(2);
		if (!buf)
			return (NULL);
		buf[0] = start[1];
		buf[1] = '\0';
		*out_len = 1;
		return (buf);
	}
	if (len >= 2 && start[0] == ']' && len > 2)
	{
		start++;
		len--;
	}
	buf = malloc(512);
	if (!buf)
		return (NULL);
	buf_pos = 0;
	i = 0;
	while (i < len && buf_pos < 500)
	{
		// Handle backslash escape inside bracket: treat next char literally
		if (start[i] == '\\' && i + 1 < len)
		{
			buf[buf_pos++] = start[i + 1];
			i += 2;
			continue;
		}
		// Check for POSIX character class [:name:]
		if (i + 1 < len && start[i] == '[' && start[i + 1] == ':')
		{
			consumed = check_posix_class(start + i, len - i, buf, &buf_pos);
			if (consumed > 0)
			{
				i += consumed;
				continue;
			}
		}
		// Handle - at start or end as literal
		if (start[i] == '-' && (i == 0 || i == len - 1))
		{
			buf[buf_pos++] = '-';
			i++;
			continue;
		}
		// Check for range like a-z (but not at start/end where - is literal)
		if (i + 2 < len && start[i + 1] == '-' && start[i + 2] != ']' && start[i + 2] != '[')
		{
			// If the dash is escaped, treat as literal
			if (start[i + 1] == '-' && start[i] == '\\')
			{
				buf[buf_pos++] = '-';
				i += 2;
				continue;
			}
			// If the dash is not escaped, but the dash itself is escaped, treat as literal
			if (start[i + 1] == '-' && start[i + 2] == '\\' && i + 3 < len)
			{
				range_count = expand_range(start[i], start[i + 3], buf, buf_pos);
				if (range_count < 0)
				{
					buf[buf_pos++] = start[i];
					buf[buf_pos++] = '-';
					i += 2;
					continue;
				}
				buf_pos += range_count;
				i += 4;
				continue;
			}
			range_count = expand_range(start[i], start[i + 2], buf, buf_pos);
			if (range_count < 0)
			{
				// invalid range: treat as literal "start" and '-' and continue with end char
				buf[buf_pos++] = start[i];
				buf[buf_pos++] = '-';
				i += 2; // skip start and '-'; do not consume the end char here
				continue;
			}
			buf_pos += range_count;
			i += 3;
			continue;
		}
		// Single character (skip if part of unrecognized [:...:] sequence)
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
bool glob_char_in_class(char c, t_glob *bracket)
{
	int i;
	bool found;

	if (!bracket->char_set || bracket->char_set_len == 0)
		return (false);
	found = false;
	i = 0;
	while (i < bracket->char_set_len)
	{
		if (bracket->char_set[i] == c)
		{
			found = true;
			break;
		}
		i++;
	}
	if (bracket->flags & BRACKET_NEGATED)
		return (!found);
	return (found);
}
