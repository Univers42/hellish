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
#include "sh_input.h"

/* Print all environment entries in `name=value` form (or just `name` when a
   variable has no value). This is the output of bare `set` — it differs from
   `env` in that it includes all variables, not just the exported ones, and
   it does not quote the values. */
/* Arrays list in bash's display form, name=([0]="a" ...), so the
   private value encoding never reaches the terminal. */
static void	print_env_array(t_env *e)
{
	char	*fmt;

	fmt = arr_format(e->value);
	if (fmt)
		ft_printf("%s=%s\n", e->key, fmt);
	xfree(fmt);
}

void	set_print_env(t_shell *state)
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
			{
				if (arr_is(e->value))
					print_env_array(e);
				else
					ft_printf("%s=%s\n", e->key, e->value);
			}
			else
				ft_printf("%s\n", e->key);
		}
		i++;
	}
}

/* set: multi-purpose.  With no args, dump the environment.  Everything else
   — short flag clusters (`set -eux`), -o/+o named options, `--`, the lone
   -/+ oddities and positional replacement — is one left-to-right scan in
   apply_set_flags, mirroring how bash and dash consume set's arguments. */
int	builtin_set(t_shell *state, t_vec argv)
{
	int	rc;

	if (argv.len < 2)
		return (set_print_env(state), 0);
	rc = apply_set_flags(state, argv);
	if (rc == 2 && state->metinp != INP_RL)
		exit_clean(state, 2);
	return (rc);
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

/* shift [n]: discard the first n positional parameters (default 1). We
   collect the remaining tail as a new owned array, then replace the current
   set. Shifting by more than $# is an error (status 1) per POSIX — we do
   NOT exit; the caller can check $? and decide.  A malformed operand is a
   different thing entirely and shift_operand() handles it: see shift_args.c
   for why the status alone cannot tell the two apart.
     zsh's `shift [n] name` shifts an ARRAY instead, and gets first refusal:
   it answers -1 for every spelling this function handles. */
int	builtin_shift(t_shell *state, t_vec argv)
{
	char	**vals;
	char	*cnt;
	int		count;
	int		n;

	count = zsh_shift_array(state, argv);
	if (count >= 0)
		return (count);
	n = shift_operand(state, argv, &count);
	if (n)
		return (n);
	n = count;
	cnt = env_expand(state, "#");
	if (cnt)
		count = ft_atoi(cnt);
	else
		count = 0;
	if (n < 0 || n > count)
		return (ft_eprintf("%s: shift: %d: shift count out of range\n",
				state->ctx, n), 1);
	vals = collect_tail_positionals(state, n + 1, count);
	if (!vals)
		return (1);
	set_positional_args(state, vals, (size_t)(count - n));
	return (free_split(vals), 0);
}
