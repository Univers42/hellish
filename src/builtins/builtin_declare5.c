/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_declare5.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

void	declare_assign(t_shell *state, const char *word, int exprt);

/* The attribute bits one option word carries: 1 -p, 2 -x, 4 -A, 8 any of
   -t/-r/-g (accepted, not tracked), 16 +x. A `+` word only ever means
   "take the attribute away"; +x is the one attribute hellish tracks. */
int	declare_flag_bits(const char *w)
{
	int	bits;

	bits = 0;
	if (w[0] == '+')
	{
		if (ft_strchr(w, 'x'))
			bits |= 16;
		return (bits);
	}
	if (ft_strchr(w, 'p'))
		bits |= 1;
	if (ft_strchr(w, 'x'))
		bits |= 2;
	if (ft_strchr(w, 'A'))
		bits |= 4;
	if (ft_strchr(w, 't') || ft_strchr(w, 'r') || ft_strchr(w, 'g'))
		bits |= 8;
	return (bits);
}

/* `declare +x NAME...` / `typeset +x NAME...`: the bash spelling of
   un-export. A `+` word was never an option to declare_scan (it only
   looked at `-`), so `+x` fell through as an operand and the real
   operands were exported harder. With a value the assignment happens
   first, then the attribute comes off, exactly as export -n does. */
int	declare_unexport(t_shell *state, t_vec argv, size_t i)
{
	const char	*w;
	char		*eq;
	char		*name;

	while (i < argv.len)
	{
		w = ((char **)argv.ctx)[i++];
		eq = ft_strchr((char *)w, '=');
		if (eq)
		{
			declare_assign(state, w, 0);
			name = ft_substr(w, 0, (size_t)(eq - w));
		}
		else
			name = ft_strdup(w);
		if (name)
			env_unexport(&state->env, name);
		xfree(name);
	}
	return (0);
}
