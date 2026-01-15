/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_types.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/15 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"
#include <stdlib.h>

/*
** Free all allocated memory in glob tokens
*/
void	glob_free_tokens(t_vec_glob *tokens)
{
	size_t	i;
	t_glob	*g;

	if (!tokens || !tokens->ctx)
		return ;
	i = 0;
	while (i < tokens->len)
	{
		g = &((t_glob *)tokens->ctx)[i];
		if (g->char_set)
		{
			free(g->char_set);
			g->char_set = NULL;
		}
		i++;
	}
	free(tokens->ctx);
	tokens->ctx = NULL;
	tokens->len = 0;
}

/*
** Check if character is a glob special character (when unquoted)
*/
bool	glob_is_special(char c)
{
	return (c == '*' || c == '?' || c == '[');
}

/*
** Check if pattern segment ends at current position
** (next is slash or end of pattern)
*/
bool	finished_pattern(t_vec_glob patt, size_t offset)
{
	t_glob	curr;

	if (offset + 1 >= patt.len)
		return (true);
	curr = ((t_glob *)patt.ctx)[offset + 1];
	if (curr.ty == G_SLASH)
		return (true);
	return (false);
}
