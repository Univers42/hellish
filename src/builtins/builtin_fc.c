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
#include "pal_wait.h"

/* Resolve one history specifier against the `total` entries fc sees (a
   0-based index into *idx). A number: positive is an absolute history
   number, negative counts back from the most recent (-1), 0 is the most
   recent; out-of-range numbers are clamped, as POSIX lets fc do. Anything
   else names the most recent command STARTING with it (`fc -l ech`), and
   one that no command starts with is an error. NULL is the most recent.
   Returns 0, or 1 after reporting. */
int	fc_resolve_idx(t_shell *state, const char *s, int total, int *idx)
{
	int		n;
	char	**ents;

	ents = (char **)state->hist.hist_cmds.ctx;
	*idx = total - 1;
	if (!s)
		return (0);
	if (*s == '-' || *s == '+' || ft_isdigit((unsigned char)*s))
	{
		n = ft_atoi(s) - 1;
		if (n < 0)
			n += total + 1;
		*idx = n;
		if (n < 0)
			*idx = 0;
		if (n >= total)
			*idx = total - 1;
		return (0);
	}
	while (*idx >= 0 && ft_strncmp(ents[*idx], s, ft_strlen(s)))
		(*idx)--;
	if (*idx >= 0)
		return (0);
	return (ft_eprintf("%s: fc: no command found\n", state->ctx), 1);
}

/* One entry, bash's layout: `N<TAB> cmd`, or `<TAB> cmd` with -n. */
static void	fc_print_entry(t_shell *state, int i, bool nonum)
{
	if (!nonum)
		ft_printf("%d", i + 1);
	ft_printf("\t %s\n", ((char **)state->hist.hist_cmds.ctx)[i]);
}

/* Print first..last, whichever way round they are: first after last
   lists backwards, and -r flips whichever direction that gave. */
static void	fc_print_range(t_shell *state, int first, int last, t_fcopt *o)
{
	int	step;
	int	swap;

	if (o->rev)
	{
		swap = first;
		first = last;
		last = swap;
	}
	step = 1 - 2 * (first > last);
	while (first != last + step)
	{
		fc_print_entry(state, first, o->nonum);
		first += step;
	}
}

/* fc -l [-nr] [first [last]]. No operand: the last 16. One: from it to
   the most recent. */
int	fc_list(t_shell *state, t_fcopt *o)
{
	int	total;
	int	first;
	int	last;

	total = fc_total(state);
	if (total <= 0)
		return (0);
	first = total - 16;
	if (first < 0)
		first = 0;
	if (fc_resolve_idx(state, NULL, total, &last)
		|| (o->nops > 0 && fc_resolve_idx(state, o->ops[0], total, &first))
		|| (o->nops > 1 && fc_resolve_idx(state, o->ops[1], total, &last)))
		return (1);
	fc_print_range(state, first, last, o);
	return (0);
}

/* Write history entries first..last to a mkstemp() temp file and leave its
   path in tmpf[0..63]. The temp file is later opened by the editor and then
   read back by fc_run_editor. Returns 1 on failure (can't create file or
   can't write); the caller must not proceed if this returns 1. */
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
