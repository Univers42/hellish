/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_which.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/18 03:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/18 03:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* zsh's `which` (its `whence -c`), a builtin in zsh mode only -- see
   zsh_builtin_func() in hash_builtins_zsh.c for why the gate. Plugins call
   it as `builtin which`: oh-my-zsh's sudo widget runs `builtin which -a`
   on every Esc Esc, and without it each toggle printed "not a shell
   builtin" over the prompt.
   -a lists every match instead of the first, -p skips aliases, reserved
   words, functions and builtins; any other letter is zsh's "bad option",
   status 1. A name nothing answers to prints "NAME not found" on stdout
   and makes the status 1, as zsh does; no operand at all is status 1. */
static int	which_opts(t_shell *state, t_vec argv, size_t *i, int *flags)
{
	char	*a;
	size_t	k;

	while (*i < argv.len)
	{
		a = ((char **)argv.ctx)[*i];
		if (a[0] != '-' || !a[1])
			break ;
		(*i)++;
		if (!ft_strcmp(a, "--"))
			break ;
		k = 0;
		while (a[++k])
		{
			if (a[k] == 'a')
				*flags |= WHICH_ALL;
			else if (a[k] == 'p')
				*flags |= WHICH_PATH;
			else
				return (ft_eprintf("%s: which: bad option: -%c\n",
						state->ctx, a[k]), 1);
		}
	}
	return (0);
}

int	builtin_which(t_shell *state, t_vec argv)
{
	size_t	i;
	int		flags;
	int		rc;

	i = 1;
	flags = 0;
	if (which_opts(state, argv, &i, &flags))
		return (1);
	rc = (i >= argv.len);
	while (i < argv.len)
		if (which_one(state, ((char **)argv.ctx)[i++], flags))
			rc = 1;
	return (rc);
}
