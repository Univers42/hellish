/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:17:47 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 14:50:30 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* consume pending skip marker if set */
static bool	consume_pending_skip(bool *skip_next)
{
	if (*skip_next)
	{
		*skip_next = false;
		return (true);
	}
	return (false);
}

/* handle redirection token, set skip_next if
the token requires a following arg */
static bool	handle_redir_token(char *arg, bool *skip_next)
{
	if (!is_redir_operator(arg))
		return (false);
	if (redir_needs_next(arg))
		*skip_next = true;
	return (true);
}

int	count_real_args(t_vec argv)
{
	size_t	i;
	int		count;
	char	*arg;
	bool	skip_next;

	count = 0;
	skip_next = false;
	i = 1;
	while (i < argv.len)
	{
		arg = ((char **)argv.ctx)[i];
		if (consume_pending_skip(&skip_next))
		{
			i++;
			continue ;
		}
		if (handle_redir_token(arg, &skip_next))
		{
			i++;
			continue ;
		}
		count++;
		i++;
	}
	return (count);
}
