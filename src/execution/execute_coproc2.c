/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_coproc2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Publish the coprocess handles: NAME becomes a two-element indexed array
   ([0]=read fd, [1]=write fd) and NAME_PID the child pid, both ordinary
   (non-exported) shell variables the script reads back. */
void	coproc_store(t_shell *state, char *name, int *fds, pid_t pid)
{
	char	*elems[2];
	char	*pidkey;

	elems[0] = ft_itoa(fds[0]);
	elems[1] = ft_itoa(fds[1]);
	env_set(&state->env, env_create(ft_strdup(name),
			arr_from_elems(elems, 2, NULL), false));
	xfree(elems[0]);
	xfree(elems[1]);
	pidkey = ft_strjoin(name, "_PID");
	env_set(&state->env, env_create(pidkey, ft_itoa((int)pid), false));
}
