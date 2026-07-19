/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_set.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 09:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 09:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Handle one `-o [name]` / `+o [name]` pair inside the set argument scan.
   `w` points at the -o/+o word itself and `remaining` counts the words left
   including it; the return value is how many words were consumed (2 when a
   name followed, 1 when -o/+o was last — then we list the options, like
   bash).  Consuming the name here is what lets `set -o errexit a b c` go on
   to make a/b/c the positional parameters.  vi/emacs are special: they
   touch both state->edit_mode and the readline layer (state->rl.edit_mode).
   Everything else goes through set_long_option which maps the long name to
   the right opt_ field. */
int	set_o_word(t_shell *state, char sign, char **w, size_t remaining)
{
	if (remaining < 2)
		return (list_set_options(state), 1);
	if (ft_strcmp(w[1], "vi") == 0)
	{
		state->edit_mode = 0;
		state->rl.edit_mode = 0;
		return (2);
	}
	if (ft_strcmp(w[1], "emacs") == 0)
	{
		state->edit_mode = 1;
		state->rl.edit_mode = 1;
		return (2);
	}
	if (ft_strcmp(w[1], "posix") == 0)
		return (state->opt_posix = (sign == '-'), 2);
	return (set_long_option(state, sign, w[1]), 2);
}

/* Cache the decimal of $# in the fixed buffer so env_expand can return it. */
void	pos_set_cnt(t_pos *pos)
{
	char	*s;

	s = ft_itoa(pos->count);
	if (s)
		ft_strlcpy(pos->cnt_str, s, sizeof(pos->cnt_str));
	else
		ft_strlcpy(pos->cnt_str, "0", sizeof(pos->cnt_str));
	xfree(s);
}

/* Fill `pos` with fresh dups of args[0..n-1]. */
void	pos_build(t_pos *pos, char **args, size_t n)
{
	size_t	i;

	pos->args = ft_calloc(n + 1, sizeof(char *));
	i = 0;
	while (pos->args && args && i < n)
	{
		pos->args[i] = ft_strdup(args[i]);
		i++;
	}
	pos->count = (int)n;
	pos->args_owned = true;
	pos_set_cnt(pos);
}

/* Release the positional array; string elements are freed only when owned (a
   borrowed set points into the caller's argv, which owns/frees the strings). */
void	pos_free(t_pos *pos)
{
	size_t	i;

	if (pos->args)
	{
		i = 0;
		if (pos->args_owned)
			while (pos->args[i])
				xfree(pos->args[i++]);
		xfree(pos->args);
	}
	pos->args = NULL;
	pos->count = 0;
	pos->args_owned = false;
}

/* Replace the shell's positional parameters with a fresh owned copy of args.
   The old set is freed by pos_free() first, then pos_build() makes deep-dup
   strings so the caller can free the original array safely. */
int	set_positional_args(t_shell *state, char **args, size_t n)
{
	pos_free(&state->pos);
	pos_build(&state->pos, args, n);
	return (0);
}
