/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_dirstack.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/04 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>

void			update_pwd_vars(t_shell *state);

static t_vec	g_dirstack;

/* After a chdir, refresh the cached cwd and PWD/OLDPWD, as cd does. */
static void	ds_refresh(t_shell *state)
{
	char	*cwd;

	cwd = getcwd(NULL, 0);
	if (cwd)
	{
		state->cwd.len = 0;
		vec_push_str(&state->cwd, cwd);
		free(cwd);
	}
	update_pwd_vars(state);
}

/* Print the stack bash-style: current dir first, then the saved dirs. */
static void	ds_print(void)
{
	char	*cwd;
	int		i;

	cwd = getcwd(NULL, 0);
	if (cwd)
		ft_printf("%s", cwd);
	free(cwd);
	i = (int)g_dirstack.len - 1;
	while (i >= 0)
	{
		ft_printf(" %s", ((char **)g_dirstack.ctx)[i]);
		i--;
	}
	ft_printf("\n");
}

int	builtin_pushd(t_shell *state, t_vec argv)
{
	char	*old;

	if (argv.len < 2)
		return (ft_eprintf("%s: pushd: directory argument required\n",
				state->ctx), 1);
	if (g_dirstack.elem_size == 0)
	{
		vec_init(&g_dirstack);
		g_dirstack.elem_size = sizeof(char *);
	}
	old = getcwd(NULL, 0);
	if (chdir(((char **)argv.ctx)[1]) != 0)
		return (free(old), ft_eprintf("%s: pushd: %s: %s\n", state->ctx,
				((char **)argv.ctx)[1], strerror(errno)), 1);
	if (old)
		vec_push(&g_dirstack, &old);
	return (ds_refresh(state), ds_print(), 0);
}

int	builtin_popd(t_shell *state, t_vec argv)
{
	char	*top;

	(void)argv;
	if (g_dirstack.len == 0)
		return (ft_eprintf("%s: popd: directory stack empty\n", state->ctx), 1);
	top = ((char **)g_dirstack.ctx)[g_dirstack.len - 1];
	g_dirstack.len--;
	if (chdir(top) != 0)
	{
		ft_eprintf("%s: popd: %s: %s\n", state->ctx, top, strerror(errno));
		return (free(top), 1);
	}
	free(top);
	ds_refresh(state);
	ds_print();
	if (g_dirstack.len == 0)
		(free(g_dirstack.ctx), g_dirstack = (t_vec){0});
	return (0);
}
