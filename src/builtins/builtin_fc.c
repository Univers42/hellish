/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_fc.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "history.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int	fc_resolve_idx(t_shell *state, const char *s)
{
	int	n;
	int	total;

	total = (int)state->hist.hist_cmds.len;
	if (!s)
		return (total - 1);
	n = ft_atoi(s);
	if (n < 0)
		n = total + n;
	else
		n = n - 1;
	if (n < 0)
		n = 0;
	if (n >= total)
		n = total - 1;
	return (n);
}

static void	fc_print_entries(t_shell *state, int first, int last, bool nonums)
{
	int	i;

	i = first - 1;
	while (++i <= last)
	{
		if (!nonums)
			ft_printf("%d\t", i + 1);
		ft_printf("%s\n", ((char **)state->hist.hist_cmds.ctx)[i]);
	}
}

static bool	fc_check_nonums(char **av, int ac)
{
	int	i;

	i = -1;
	while (++i < ac)
		if (av[i] && ft_strcmp(av[i], "-n") == 0)
			return (true);
	return (false);
}

int	fc_list(t_shell *state, char **av, int ac, bool reverse)
{
	int		first;
	int		last;
	int		tmp;

	first = fc_resolve_idx(state, NULL);
	if (ac > 0)
		first = fc_resolve_idx(state, av[0]);
	last = fc_resolve_idx(state, NULL);
	if (ac > 1)
		last = fc_resolve_idx(state, av[1]);
	if (first == last && ac < 2)
	{
		first = (int)state->hist.hist_cmds.len - 16;
		if (first < 0)
			first = 0;
		last = (int)state->hist.hist_cmds.len - 1;
	}
	if (reverse && first < last)
	{
		tmp = first;
		first = last;
		last = tmp;
	}
	fc_print_entries(state, first, last, fc_check_nonums(av, ac));
	return (0);
}

int	fc_write_tmp(t_shell *state, char *tmpf, int first, int last)
{
	int		fd;
	int		i;
	char	*entry;

	ft_strlcpy(tmpf, "/tmp/.hellish_fc_XXXXXX", 64);
	fd = mkstemp(tmpf);
	if (fd < 0)
		return (ft_eprintf("%s: fc: cannot create temp file\n",
				state->ctx), 1);
	i = first;
	while (i <= last)
	{
		entry = ((char **)state->hist.hist_cmds.ctx)[i];
		if (write(fd, entry, ft_strlen(entry)) < 0)
			break ;
		if (write(fd, "\n", 1) < 0)
			break ;
		i++;
	}
	close(fd);
	return (0);
}
