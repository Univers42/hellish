/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:27:39 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 14:56:13 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

int	parse_redir_len(const char *arg)
{
	int	j;

	if (!arg || !*arg)
		return (0);
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

bool	redir_needs_next(const char *arg)
{
	int	len;

	if (!arg)
		return (false);
	len = parse_redir_len(arg);
	if (arg[len] == '\0')
		return (true);
	return (false);
}

bool	is_redir_operator(char *s)
{
	int	i;

	if (!s || !*s)
		return (false);
	if (*s == '<' || *s == '>')
		return (true);
	i = 0;
	while (ft_isdigit((unsigned char)s[i]))
		i++;
	if (s[i] == '<' || s[i] == '>')
		return (true);
	return (false);
}

int	check_args(t_vec argv)
{
	int	real_args;

	real_args = count_real_args(argv);
	if (real_args >= 2)
		return (1);
	return (0);
}
