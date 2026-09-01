/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_builtin.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/01 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* builtin [name [args...]]: run the shell builtin `name` directly,
   bypassing function lookup. That bypass is the entire point — real
   plugin code shadows builtins with functions and still needs the
   original (bash-preexec's hook runs `builtin history 1` so a user
   function named `history` cannot intercept it; that call is what made
   this builtin necessary once #96 let DEBUG traps installed from
   functions actually fire). With no operands the status is 0; an
   unknown name is "not a shell builtin", status 1, both as bash.
   The argv handed down is a window one slot into ours — safe because
   no builtin frees or grows its argv vec. */
int	builtin_builtin(t_shell *state, t_vec argv)
{
	int		(*f)(t_shell *, t_vec);
	t_vec	sub;

	if (argv.len < 2)
		return (0);
	f = builtin_func(((char **)argv.ctx)[1]);
	if (!f)
	{
		ft_eprintf("%s: builtin: %s: not a shell builtin\n",
			state->ctx, ((char **)argv.ctx)[1]);
		return (1);
	}
	sub = argv;
	sub.ctx = (char **)argv.ctx + 1;
	sub.len = argv.len - 1;
	return (f(state, sub));
}
