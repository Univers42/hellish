/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_history.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "history.h"

/* history [-c] [-d offset] [n]
   history -a|-n|-r|-w [file]
   history -p|-s arg...

   The list-only version of this builtin used to swallow every option word:
   ft_atoi("-a") is 0, 0 means "no count", and "no count" means print the
   whole list. That is issue #42. `PROMPT_COMMAND='history -a'` is a stock
   bashrc line, hellish imports PROMPT_COMMAND from the environment like
   bash does, and so every single prompt dumped the entire history file to
   the terminal. The reporter's shell was unusable and nothing they typed
   caused it.

   So options are parsed for real now, and the ones bash defines all work. */

/* Fold one option character into the parsed request. The four file
   operations are mutually exclusive -- `history -anrw` is an error, not a
   request to do all four. Returns 0, or the status to exit with. */
static int	hist_opt_char(t_shell *state, t_histopt *o, char c)
{
	if (c == 'a' || c == 'n' || c == 'r' || c == 'w')
	{
		if (o->fileop && o->fileop != c)
			return (ft_eprintf("%s: history: cannot use more than one of"
					" -anrw\n", state->ctx), 1);
		o->fileop = c;
		return (0);
	}
	if (c == 'c')
		return (o->clear = true, 0);
	if (c == 'd' || c == 'p' || c == 's')
		return (o->act = c, 0);
	return (ft_eprintf("%s: history: -%c: invalid option\n",
			state->ctx, c), 2);
}

/* Walk the option words. Parsing stops at the first non-option word, at a
   lone "--", or immediately after -p / -s, whose every remaining word is an
   operand (`history -p -c` prints "-c", it does not clear). */
static int	hist_parse(t_shell *state, t_vec argv, t_histopt *o)
{
	char	**av;
	int		j;
	int		st;

	av = (char **)argv.ctx;
	o->first = 1;
	while (o->first < (int)argv.len && av[o->first][0] == '-'
		&& av[o->first][1])
	{
		if (!ft_strcmp(av[o->first], "--"))
			return (o->first++, 0);
		j = 0;
		while (av[o->first][++j])
		{
			st = hist_opt_char(state, o, av[o->first][j]);
			if (st)
				return (st);
			if (o->act == 'p' || o->act == 's')
				return (o->first++, 0);
		}
		o->first++;
	}
	return (0);
}

/* history [n] -- print the list, newest n entries only when n is given.
   A non-numeric operand is a usage error (status 2), NOT a silent 0; that
   silence is what let `-a` through as "print everything". */
static int	hist_show(t_shell *state, t_vec argv, int first)
{
	char	**av;
	int		n;
	int		i;

	av = (char **)argv.ctx;
	i = 0;
	if (first < (int)argv.len)
	{
		if (ft_checked_atoi(av[first], &n, 0) < 0)
			return (ft_eprintf("%s: history: %s: numeric argument"
					" required\n", state->ctx, av[first]), 2);
		if (n > 0 && n < (int)state->hist.hist_cmds.len)
			i = (int)state->hist.hist_cmds.len - n;
	}
	while (i < (int)state->hist.hist_cmds.len)
	{
		ft_printf("%5d  %s\n", i + 1,
			((char **)state->hist.hist_cmds.ctx)[i]);
		i++;
	}
	return (0);
}

/* Run the requested actions in bash's order: -c, then -d, then at most one
   file operation, then -p / -s, and only otherwise the plain listing. */
int	builtin_history(t_shell *state, t_vec argv)
{
	t_histopt	o;
	int			st;

	o = (t_histopt){0};
	st = hist_parse(state, argv, &o);
	if (st)
		return (st);
	hist_list_init(state);
	if (o.clear)
		hist_clear(state);
	if (o.act == 'd')
		return (hist_delete(state, argv, o.first));
	if (o.fileop)
		return (hist_fileop(state, argv, &o));
	if (o.act == 'p')
		return (hist_expand_args(state, argv, o.first));
	if (o.act == 's')
		return (hist_store(state, argv, o.first));
	return (hist_show(state, argv, o.first));
}
