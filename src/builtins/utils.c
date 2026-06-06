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

/* If skip_next is set, clear it and return true (skip this word — it is the
   target of a redirection operator from the previous word). */
static bool	consume_pending_skip(bool *skip_next)
{
	if (*skip_next)
	{
		*skip_next = false;
		return (true);
	}
	return (false);
}

/* If `arg` is a redirection operator, set skip_next when the operator needs
   a target word (e.g. ">" alone vs ">file" with the path attached). Returns
   true so the caller knows to skip this word in the real-arg count. */
static bool	handle_redir_token(char *arg, bool *skip_next)
{
	if (!is_redir_operator(arg))
		return (false);
	if (redir_needs_next(arg))
		*skip_next = true;
	return (true);
}

/* Count the arguments in argv that are actual command operands — skipping
   -L/-P options and redirection tokens (plus their targets). Used by
   check_args() to decide if `cd` has been given too many real arguments. */
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
		if (consume_pending_skip(&skip_next)
			|| handle_redir_token(arg, &skip_next)
			|| !ft_strcmp(arg, "-L") || !ft_strcmp(arg, "-P"))
		{
			i++;
			continue ;
		}
		count++;
		i++;
	}
	return (count);
}
