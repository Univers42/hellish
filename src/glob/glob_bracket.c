/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_bracket.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 13:14:51 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/*
** Expand a character range like a-z into individual characters
** Returns number of characters added to buf
*/
int	expand_range(char start, char end, char *buf, int buf_pos)
{
	char	c;
	int		count;

	count = 0;
	if (start > end)
		return (-1);
	c = start;
	while (c <= end)
	{
		buf[buf_pos + count] = c;
		count++;
		c++;
	}
	return (count);
}

int	check_posix_class(const char *s, int len, char *buf, int *buf_pos)
{
	int						i;
	size_t					chars_len;
	const struct s_classes	*classes = get_classes_singleton();

	if (len < 2 || s[0] != '[' || s[1] != ':')
		return (0);
	i = 0;
	while (classes[i].pattern)
	{
		if (len >= classes[i].plen
			&& ft_strncmp(s, classes[i].pattern, classes[i].plen) == 0)
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
