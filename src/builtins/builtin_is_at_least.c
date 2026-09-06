/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_is_at_least.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"

/* is-at-least NEED [HAVE] -- zsh's version comparison, as a builtin.
**
** zsh ships it as an autoloadable function; hellish has no zsh fpath to
** load it from, so oh-my-zsh's git plugin -- which calls it three times
** to pick between the `git switch` and `git checkout` spellings --
** printed "is-at-least: command not found" at every shell start for
** anyone with plugins=(git).  The comparison is numeric per dotted
** component, a missing component counts as 0, and a tail that is not a
** number (5.9-dev, 2.30.0rc1) compares as text once the numbers agree.
** HAVE defaults to $ZSH_VERSION, which is unset here on purpose (see
** zsh_mode.c): with nothing to compare, the answer is "no", status 1,
** which is what zsh's function returns for an empty version too.
*/

/* The next dotted component as a number; *p moves past it and its dot. */
static long	ver_part(const char **p)
{
	long	n;

	n = 0;
	while (ft_isdigit((unsigned char)**p))
	{
		n = n * 10 + (**p - '0');
		(*p)++;
	}
	if (**p == '.')
		(*p)++;
	return (n);
}

/* <0, 0, >0 as a is older than, the same as, newer than b. */
static int	ver_cmp(const char *a, const char *b)
{
	long	x;
	long	y;

	while (ft_isdigit((unsigned char)*a) || ft_isdigit((unsigned char)*b))
	{
		x = ver_part(&a);
		y = ver_part(&b);
		if (x != y)
			return ((x > y) - (x < y));
	}
	return (ft_strcmp(a, b));
}

int	builtin_is_at_least(t_shell *state, t_vec argv)
{
	char	**av;
	char	*have;

	av = (char **)argv.ctx;
	if (argv.len < 2)
		return (ft_eprintf("%s: is-at-least: not enough arguments\n",
				state->ctx), 2);
	if (argv.len > 2)
		have = av[2];
	else
		have = env_expand(state, "ZSH_VERSION");
	if (!have || !*have)
		return (1);
	return (ver_cmp(have, av[1]) < 0);
}
