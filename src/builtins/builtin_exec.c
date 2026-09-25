/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_exec.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "sh_error.h"
#include "sys.h"
#include "zle.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void	procsub_detach_all(t_shell *state);

/* argv[first..] as execve's NULL-terminated array. The strings are not
   copied: execve does not need them to outlive the image, and after a
   failed exec they still belong to the command. */
static char	**exec_argv(t_vec argv, size_t first)
{
	char	**out;
	size_t	i;

	out = ft_calloc(argv.len - first + 1, sizeof(char *));
	if (!out)
		return (NULL);
	i = first;
	while (i < argv.len)
	{
		out[i - first] = ((char **)argv.ctx)[i];
		i++;
	}
	return (out);
}

/* A failed exec ends a shell that is not interactive -- a script, -c, any
   subshell -- with its status, the EXIT trap run first; an interactive
   shell only returns the status (bash's execfail rule, less the option). */
static int	exec_failed(t_shell *state, int status)
{
	if (sh_interactive(state) && getpid() == state->shell_pid)
		return (status);
	exit_clean(state, status);
	return (status);
}

/* Replace the shell with `path`. Should execve return, say why the way
   bash does -- a directory is EACCES to execve, "Is a directory" to the
   user -- and give 127 for a file that is not there, 126 otherwise. */
static int	exec_run(t_shell *state, char *path, t_vec argv, size_t first)
{
	char		**xargv;
	char		**envp;
	struct stat	st;
	int			err;

	xargv = exec_argv(argv, first);
	envp = get_envp(state, path);
	pal_editor_leave();
	execve(path, xargv, envp);
	err = errno;
	xfree(xargv);
	free_tab(envp);
	if (err == EACCES && stat(path, &st) == 0 && S_ISDIR(st.st_mode))
		err = EISDIR;
	ft_eprintf("%s: %s: %s\n", state->ctx, path, strerror(err));
	if (err == ENOENT)
		return (EXIT_CMD_NOT_FOUND);
	return (EXIT_CMD_NOT_EXEC);
}

/* exec [--] [command [args]]: replace the shell with command, or, with no
   command, keep the redirections already applied. Run from a widget, a
   failed execve has already handed the terminal over, and the editor
   cannot have it back: that shell exits, as it always did. */
int	builtin_exec(t_shell *state, t_vec argv)
{
	char	*path;
	char	*name;
	size_t	first;
	int		status;
	bool	widget;

	first = 1;
	if (argv.len > 1 && ft_strcmp(((char **)argv.ctx)[1], "--") == 0)
		first = 2;
	if (argv.len <= first)
		return (procsub_detach_all(state), 0);
	name = ((char **)argv.ctx)[first];
	path = exec_lookup(state, name);
	if (!path)
	{
		ft_eprintf("%s: exec: %s: not found\n", state->ctx, name);
		return (exec_failed(state, EXIT_CMD_NOT_FOUND));
	}
	widget = zle_active();
	status = exec_run(state, path, argv, first);
	xfree(path);
	if (widget)
		exit_clean(state, status);
	return (exec_failed(state, status));
}
