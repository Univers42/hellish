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

/* The directory stack lives in t_shell (state->dirstack), not a file-scope
   global -- the norm allows exactly one global (the signal), and threading it
   through state keeps subshells honest. Elements are heap-allocated path
   strings; pushd owns them, popd frees them, and free_all_state() drops any
   that survive to exit so a never-popped stack does not leak. */

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

/* Print the stack bash-style: current dir first, then the saved dirs.
   Not static: `dirs` (builtin_dirs.c) is exactly this, and pushd/popd have
   been printing through it all along -- so exposing it costs nothing and
   guarantees the three stay byte-identical. */
void	dirstack_print(t_shell *state)
{
	char	*cwd;
	int		i;

	cwd = x_getcwd();
	if (cwd)
		ft_printf("%s", cwd);
	xfree(cwd);
	i = (int)state->dirstack.len - 1;
	while (i >= 0)
	{
		ft_printf(" %s", ((char **)state->dirstack.ctx)[i]);
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
	if (state->dirstack.elem_size == 0)
	{
		vec_init(&state->dirstack);
		state->dirstack.elem_size = sizeof(char *);
	}
	old = x_getcwd();
	if (chdir(((char **)argv.ctx)[1]) != 0)
		return (xfree(old), ft_eprintf("%s: pushd: %s: %s\n", state->ctx,
				((char **)argv.ctx)[1], strerror(errno)), 1);
	if (old)
		vec_push(&state->dirstack, &old);
	return (ds_refresh(state), dirstack_print(state), 0);
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
	if (state->dirstack.len == 0)
		return (ft_eprintf("%s: popd: directory stack empty\n", state->ctx), 1);
	top = ((char **)state->dirstack.ctx)[state->dirstack.len - 1];
	state->dirstack.len--;
	if (chdir(top) != 0)
	{
		ft_eprintf("%s: popd: %s: %s\n", state->ctx, top, strerror(errno));
		return (xfree(top), 1);
	}
	xfree(top);
	ds_refresh(state);
	dirstack_print(state);
	if (state->dirstack.len == 0)
		(xfree(state->dirstack.ctx), state->dirstack = (t_vec){0});
	return (0);
}

/* Drop the whole directory stack at shutdown: free each saved path, then the
   backing array. popd frees as it pops, but a stack that is pushed and never
   fully popped would otherwise leak its survivors at exit. */
void	free_dirstack(t_shell *state)
{
	size_t	i;

	i = 0;
	while (i < state->dirstack.len)
		xfree(((char **)state->dirstack.ctx)[i++]);
	xfree(state->dirstack.ctx);
	state->dirstack = (t_vec){0};
}
