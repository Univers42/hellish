/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_set2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Print all environment entries in `name=value` form (or just `name` when a
   variable has no value). This is the output of bare `set` — it differs from
   `env` in that it includes all variables, not just the exported ones, and
   it does not quote the values. */
static void	set_print_env(t_shell *state)
{
	size_t	i;
	t_env	*e;

	i = 0;
	while (i < state->env.len)
	{
		e = &((t_env *)state->env.ctx)[i];
		if (e->key)
		{
			if (e->value)
				ft_printf("%s=%s\n", e->key, e->value);
			else
				ft_printf("%s\n", e->key);
		}
		i++;
	}
}

/* set: multi-purpose. With no args, dump the environment. With -o/+o toggle
   named options (vi/emacs/errexit/…). With -e, -u, -x, etc., toggle the
   short flag letters (can be combined: `set -eux`). With `--` (or no dash),
   replace the positional parameters. */
int	builtin_set(t_shell *state, t_vec argv)
{
	char	**av;

	av = (char **)argv.ctx;
	if (argv.len >= 2 && ft_strcmp(av[1], "-o") == 0)
		return (handle_set_o(state, argv));
	if (argv.len >= 2 && ft_strcmp(av[1], "+o") == 0)
		return (handle_set_o(state, argv));
	if (argv.len >= 2 && ft_strcmp(av[1], "--") == 0)
		return (set_positional_args(state, av + 2, argv.len - 2));
	if (argv.len >= 2 && (av[1][0] == '-' || av[1][0] == '+') && av[1][1])
		return (apply_set_flags(state, argv));
	if (argv.len >= 2 && av[1][0] != '-' && av[1][0] != '+')
		return (set_positional_args(state, av + 1, argv.len - 1));
	if (argv.len >= 2)
		return (0);
	set_print_env(state);
	return (0);
}

/* Build a fresh array of the positionals from index `from` to `count`.
   Used by shift to construct the post-shift list without re-reading from the
   live positional array (which would change underneath us if the loop is not
   careful). */
static char	**collect_tail_positionals(t_shell *state, int from, int count)
{
	char	**vals;
	char	*k;
	char	*v;
	int		i;

	vals = ft_calloc((size_t)(count - from + 2), sizeof(char *));
	if (!vals)
		return (NULL);
	i = 0;
	while (from + i <= count)
	{
		k = ft_itoa(from + i);
		v = env_expand(state, k);
		if (v)
			vals[i] = ft_strdup(v);
		else
			vals[i] = ft_strdup("");
		xfree(k);
		i++;
	}
	return (vals);
}

/* Like pos_build but the strings are BORROWED (pointer copy, no strdup): used
   for a function call's $1.. which live in the caller's argv for the call's
   duration. shift/set promote to an owned copy (pos_build) before mutating, so
   the borrowed strings are never freed by pos_free. */
void	pos_borrow(t_pos *pos, char **args, size_t n)
{
	size_t	i;

	pos->args = ft_calloc(n + 1, sizeof(char *));
	i = 0;
	while (pos->args && args && i < n)
	{
		pos->args[i] = args[i];
		i++;
	}
	pos->count = (int)n;
	pos->args_owned = false;
	pos_set_cnt(pos);
}

/* shift [n]: discard the first n positional parameters (default 1). We
   collect the remaining tail as a new owned array, then replace the current
   set. Shifting by more than $# is an error (status 1) per POSIX — we do
   NOT exit; the caller can check $? and decide. */
int	builtin_shift(t_shell *state, t_vec argv)
{
	char	**vals;
	char	*cnt;
	int		count;
	int		n;

	cnt = env_expand(state, "#");
	if (cnt)
		count = ft_atoi(cnt);
	else
		count = 0;
	n = 1;
	if (argv.len >= 2)
		n = ft_atoi(((char **)argv.ctx)[1]);
	if (n < 0 || n > count)
		return (1);
	vals = collect_tail_positionals(state, n + 1, count);
	if (!vals)
		return (1);
	set_positional_args(state, vals, (size_t)(count - n));
	n = -1;
	while (vals[++n])
		xfree(vals[n]);
	return (xfree(vals), 0);
}
