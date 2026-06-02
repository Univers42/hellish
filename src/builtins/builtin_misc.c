/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_misc.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <sys/stat.h>

int	builtin_true(t_shell *state, t_vec argv)
{
	(void)state;
	(void)argv;
	return (0);
}

int	builtin_false(t_shell *state, t_vec argv)
{
	(void)state;
	(void)argv;
	return (1);
}

static int	parse_octal(const char *s, mode_t *out)
{
	mode_t	m;

	m = 0;
	if (!*s)
		return (-1);
	while (*s)
	{
		if (*s < '0' || *s > '7')
			return (-1);
		m = m * 8 + (*s - '0');
		s++;
	}
	*out = m;
	return (0);
}

/* umask [mode]: print (octal) or set the file-creation mask. */
int	builtin_umask(t_shell *state, t_vec argv)
{
	char	**av;
	mode_t	m;

	av = (char **)argv.ctx;
	if (argv.len >= 2 && ft_strcmp(av[1], "-S") == 0)
		return (umask_symbolic());
	if (argv.len < 2)
	{
		m = umask(0);
		umask(m);
		return (ft_printf("0%c%c%c\n", '0' + ((m >> 6) & 7),
				'0' + ((m >> 3) & 7), '0' + (m & 7)), 0);
	}
	if (parse_octal(av[1], &m) != 0)
		return (ft_eprintf("%s: umask: %s: invalid octal number\n",
				state->ctx, av[1]), 1);
	umask(m);
	return (0);
}
