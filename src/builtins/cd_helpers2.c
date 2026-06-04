/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cd_helpers2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:21:58 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 14:35:23 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

static int	redir_parsed_len(const char *arg)
{
	int	j;

	j = 0;
	while (ft_isdigit((unsigned char)arg[j]))
		j++;
	if (arg[j] == '<' || arg[j] == '>')
	{
		j++;
		if (arg[j] == '>' || arg[j] == '<')
			j++;
	}
	return (j);
}

static bool	redir_requires_next(const char *arg)
{
	int	j;

	j = redir_parsed_len(arg);
	if (arg[j] == '\0')
		return (true);
	return (false);
}

/* True if `arg` is not a cd operand: a pending skip target, a redirection
   operator (possibly consuming the next word), or a -L/-P option. */
static bool	cd_skip_arg(char *arg, bool *skip_next)
{
	if (*skip_next)
	{
		*skip_next = false;
		return (true);
	}
	if (is_redir_operator(arg))
	{
		if (redir_requires_next(arg))
			*skip_next = true;
		return (true);
	}
	return (!ft_strcmp(arg, "-L") || !ft_strcmp(arg, "-P"));
}

char	*get_first_real_arg(t_vec argv)
{
	size_t	i;
	char	*arg;
	bool	skip_next;

	skip_next = false;
	i = 1;
	while (i < argv.len)
	{
		arg = ((char **)argv.ctx)[i];
		if (!cd_skip_arg(arg, &skip_next))
			return (arg);
		i++;
	}
	return (NULL);
}
