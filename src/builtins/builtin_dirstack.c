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

/* The directory stack lives in a file-scope static vec. We use a static
   rather than putting it in t_shell to keep the struct smaller and because
   the stack is only meaningful in the interactive session anyway — subshells
   do not inherit it. Elements are heap-allocated path strings; pushd owns
   them and popd frees them. */
static t_vec	g_dirstack;

/* After a chdir, refresh the cached cwd and PWD/OLDPWD, as cd does. */
static void	ds_refresh(t_shell *state)
{
	char	*cwd;

	cwd = x_getcwd();
	if (cwd)
	{
		state->cwd.len = 0;
		vec_push_str(&state->cwd, cwd);
		xfree(cwd);
	}
	update_pwd_vars(state);
}

/* Print the stack bash-style: current dir first, then the saved dirs. */
static void	ds_print(void)
{
	char	*cwd;
	int		i;

	cwd = x_getcwd();
	if (cwd)
		ft_printf("%s", cwd);
	xfree(cwd);
	i = (int)g_dirstack.len - 1;
	while (i >= 0)
	{
		ft_printf(" %s", ((char **)g_dirstack.ctx)[i]);
		i--;
	}
	ft_printf("\n");
}

/* pushd dir: save the current directory onto the stack and cd to `dir`.
   If the stack vec has never been used, initialise it here (lazy-init keeps
   the startup path clean). The old cwd is pushed only after a successful
   chdir so a failed pushd leaves the stack unchanged. */
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
	old = x_getcwd();
	if (chdir(((char **)argv.ctx)[1]) != 0)
		return (xfree(old), ft_eprintf("%s: pushd: %s: %s\n", state->ctx,
				((char **)argv.ctx)[1], strerror(errno)), 1);
	if (old)
		vec_push(&g_dirstack, &old);
	return (ds_refresh(state), ds_print(), 0);
}

/* popd: pop the top of the directory stack and cd back to it. We remove the
   entry from the stack before the chdir so a failed chdir re-exposes the
   old stack top — the stack never holds a directory we could not enter. If
   the stack becomes empty after the pop, release its backing memory to avoid
   a leak on exit. */
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
		return (xfree(top), 1);
	}
	xfree(top);
	ds_refresh(state);
	ds_print();
	if (g_dirstack.len == 0)
		(xfree(g_dirstack.ctx), g_dirstack = (t_vec){0});
	return (0);
}
