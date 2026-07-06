/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_cmd2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "executor.h"

void	exit_clean(t_shell *state, int code);

/* return [n]: unwind the current function or sourced-file execution and
   report status n. We set state->func_return = 1 so the executor's call
   stack knows to stop running the function body without exiting the shell.
   The `& 0xFF` mask keeps the exit status in 0-255 — same as bash.
   Outside any function or sourced script, POSIX makes return an error: a
   non-interactive shell exits with status 2 (bash --posix parity), an
   interactive one just reports it and keeps going. */
int	builtin_return(t_shell *state, t_vec argv)
{
	char	*cur;
	int		n;

	if (state->func_depth == 0 && state->source_depth == 0)
	{
		ft_eprintf("%s: return: can only `return' from a function "
			"or sourced script\n", state->ctx);
		if (state->metinp != INP_RL)
			exit_clean(state, 2);
		return (2);
	}
	cur = env_expand(state, "?");
	if (cur)
		n = ft_atoi(cur);
	else
		n = 0;
	if (argv.len >= 2)
		n = ft_atoi(((char **)argv.ctx)[1]);
	state->func_return = 1;
	return (n & 0xFF);
}

/* Join argv[from..] into a space-separated string to feed to exec_string.
   Used by command_run when we need to fall back to re-parsing an external
   command invocation (the builtin path takes a different route). */
static char	*join_from(t_vec argv, size_t from)
{
	char	**av;
	char	*acc;
	char	*tmp;

	av = (char **)argv.ctx;
	acc = ft_strdup("");
	while (from < argv.len)
	{
		tmp = ft_strjoin(acc, av[from]);
		xfree(acc);
		acc = tmp;
		if (++from < argv.len)
		{
			tmp = ft_strjoin(acc, " ");
			xfree(acc);
			acc = tmp;
		}
	}
	return (acc);
}

/* Silent PATH search for command -v. */
static int	command_v(t_shell *state, char *name)
{
	char	*path;
	char	**dirs;
	int		perm;

	if (builtin_func(name) || func_lookup(state, name))
		return (ft_printf("%s\n", name), 0);
	if (ft_strchr(name, '/'))
	{
		if (access(name, X_OK) == 0)
			return (ft_printf("%s\n", name), 0);
		return (1);
	}
	path = env_expand(state, "PATH");
	if (!path)
		return (1);
	dirs = ft_split(path, ':');
	if (!dirs)
		return (1);
	perm = 0;
	path = exe_path(dirs, name, &perm);
	free_tab(dirs);
	if (path)
		return (ft_printf("%s\n", path), xfree(path), 0);
	return (1);
}

/* Run the command at argv[start..], bypassing function lookup. If it is a
   builtin, invoke it directly. Otherwise re-join the words and hand them to
   exec_string so the full pipeline/redirect machinery runs. */
static int	command_run(t_shell *state, t_vec argv, size_t start)
{
	char	**av;
	t_vec	sub;
	size_t	i;
	char	*cur;

	av = (char **)argv.ctx;
	vec_init(&sub);
	sub.elem_size = sizeof(char *);
	i = start;
	while (i < argv.len)
		vec_push(&sub, &av[i++]);
	if (builtin_func(av[start]))
		return (i = builtin_func(av[start])(state, sub), xfree(sub.ctx), i);
	xfree(sub.ctx);
	cur = join_from(argv, start);
	i = exec_string(state, cur);
	return (xfree(cur), i);
}

/* command [-p] [-v|-V] cmd [args]: run cmd bypassing functions. */
int	builtin_command(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	start;

	av = (char **)argv.ctx;
	start = 1;
	while (start < argv.len && !ft_strcmp(av[start], "-p"))
		start++;
	if (start + 1 < argv.len && (!ft_strcmp(av[start], "-v")
			|| !ft_strcmp(av[start], "-V")))
		return (command_v(state, av[start + 1]));
	if (start >= argv.len)
		return (0);
	return (command_run(state, argv, start));
}
