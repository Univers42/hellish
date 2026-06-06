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

/* Measure the length of a redirection token prefix (the part that is NOT
   the target path): optional digits + '<' or '>' + optional second '<'/'>.
   Returns 0 when the argument does not start like a redirection. This is
   the shared implementation used by both the cd and utils paths. */
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

/* True if `arg` is a bare redirection operator with no attached path: the
   next word in argv is the target. e.g. ">" needs next, ">file" does not. */
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

/* True if `s` starts with a redirection operator: an optional fd number
   followed by '<' or '>'. Used by cd and count_real_args to skip redirection
   tokens that the executor may leave in the argv of builtins. */
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

/* Return 1 if the argv contains two or more real (non-redirect, non-option)
   arguments — used by builtin_cd to detect the "too many arguments" error. */
int	check_args(t_vec argv)
{
	int	real_args;

	real_args = count_real_args(argv);
	if (real_args >= 2)
		return (1);
	return (0);
}
