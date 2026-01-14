/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pattern_matcher.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:51 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:51 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdbool.h>
#include <stddef.h>
#include "shell.h"
#include "libft.h"
# include "glob.h"

bool	finished_pattern(t_vec_glob patt, size_t offset)
{
	t_glob	curr;

	if (patt.len == offset + 1)
		return (true);
	curr = ((t_glob *)patt.ctx)[offset + 1];
	if (curr.ty == G_SLASH)
		return (true);
	return (false);
}

size_t	match_g_pattern(char *name, t_vec_glob patt, size_t offset, bool first)
{
	t_glob	curr;
	char	*orig_name;

	orig_name = name;
	while (offset < patt.len && ((t_glob *)patt.ctx)[offset].ty == G_PATTERN)
	{
		curr = ((t_glob *)patt.ctx)[offset];
		if (ft_strncmp(curr.start, name, curr.len) != 0)
			return (0);
		if (finished_pattern(patt, offset))
		{
			if (name[curr.len] == 0)
				return (offset + 1);
			return (0);
		}
		offset++;
		name += curr.len;
	}
	if (first && (!ft_strcmp(orig_name, "..") || !ft_strcmp(orig_name, "."))
		&& offset != patt.len && ((t_glob *)patt.ctx)[offset].ty != G_SLASH)
		return (0);
	return (matches_pattern(name, patt, offset, false));
}

size_t	match_g_astrisk(char *name, t_vec_glob patt, int offset, bool first)
{
	int	res;

	if (first && *name == '.')
		return (0);
	while (*name)
	{
		if (!finished_pattern(patt, offset))
		{
			res = matches_pattern(name, patt, offset + 1, false);
			if (res)
				return (res);
		}
		name++;
	}
	if (finished_pattern(patt, offset))
		return (offset + 1);
	return (0);
}

size_t	matches_pattern(char *name, t_vec_glob patt, size_t offset, bool first)
{
	t_glob	curr;

	curr = ((t_glob *)patt.ctx)[offset];
	if (curr.ty == G_PATTERN)
		return (match_g_pattern(name, patt, offset, first));
	else if (curr.ty == G_ASTERISK)
		return (match_g_astrisk(name, patt, offset, first));
	return (0);
}
