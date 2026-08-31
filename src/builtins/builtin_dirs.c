/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_dirs.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 13:45:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/27 13:45:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "ft_builtins.h"

/* dirs: print the directory stack.
**
** pushd and popd both shipped; dirs -- the third of the set, and the only
** one that lets you SEE the stack -- did not (issue #71 item 5.12). The
** printer already existed and was already byte-identical to bash: pushd and
** popd have been calling it after every push and pop. It was just static, so
** there was no way to ask for it on its own.
**
** -c empties the stack. The +N/-N selectors bash also accepts are not here;
** they are the part nobody was blocked on, and a wrong answer would be worse
** than a missing one. */
int	builtin_dirs(t_shell *state, t_vec argv)
{
	if (argv.len > 1 && !ft_strcmp(((char **)argv.ctx)[1], "-c"))
	{
		free_dirstack(state);
		return (0);
	}
	if (argv.len > 1 && ((char **)argv.ctx)[1][0] == '-')
		return (ft_eprintf("%s: dirs: %s: invalid option\n", state->ctx,
				((char **)argv.ctx)[1]), 2);
	return (dirstack_print(state), 0);
}
