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
   followed by '<' or '>'. Used by cd's operand scan (cd_collect_ops) to skip
   any redirection tokens that might leak into the argv of a builtin. */
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

/* The operand half of `unset`, split out of builtin_unset to keep it
   inside the line budget.  Walks the names left to right and ORs their
   statuses together: a read-only variable makes try_unset report and
   return 1, and unset being a special builtin that then aborts a
   non-interactive shell, the status has to survive the rest of the list
   rather than the loop stopping at the first refusal. */
int	unset_operands(t_shell *state, t_vec argv, size_t i, int fmode)
{
	char	**av;
	int		rc;

	av = (char **)argv.ctx;
	rc = 0;
	while (i < argv.len)
	{
		if (fmode)
			unset_function(state, av[i]);
		else
			rc |= try_unset(state, av[i]);
		i++;
	}
	return (rc);
}
