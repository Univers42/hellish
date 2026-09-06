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
#include "sh_alias.h"

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

/* command -v NAME: the one line bash prints -- an alias as its `alias`
   definition, a keyword, function or builtin as its name, a file as its
   path.  exe_path_preferred, not exe_path: like bash, a name that is on
   PATH but not executable is still named (and exits 0) outside --posix.
   1 when nothing was found. */
static int	command_v(t_shell *state, char *name)
{
	char	*path;
	char	**dirs;

	if (alias_get(&state->aliases, name))
		return (ft_printf("alias "), alias_print_one(&state->aliases, name));
	if (type_is_keyword(name) || builtin_func(name) || func_lookup(state, name))
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
	path = exe_path_preferred(dirs, name, state->opt_posix);
	free_tab(dirs);
	if (path)
		return (ft_printf("%s\n", path), xfree(path), 0);
	return (1);
}

/* command -v / -V over EVERY name, not just the first.  -v prints one
   line per name that is found; -V prints type's long description and
   reports a miss on stderr.  The status is 0 when ANY name was found --
   bash's any_found -- so `command -v missing ls` exits 0, and so does a
   bare `command -v` with no name at all. */
static int	command_describe(t_shell *state, t_vec argv, size_t i, bool lng)
{
	char	**av;
	int		found;

	av = (char **)argv.ctx;
	found = 0;
	while (i < argv.len)
	{
		if (lng && type_one(state, av[i]) == 0)
			found = 1;
		else if (!lng && command_v(state, av[i]) == 0)
			found = 1;
		i++;
	}
	return (found == 0 && i > 0 && argv.len > 2);
}

/* Run the command at argv[start..], bypassing function lookup. If it is a
   builtin, invoke it directly; otherwise fork and execve the words we
   already hold.
     This used to re-join those words with spaces and feed the result back
   through exec_string -- a SECOND trip through the lexer, expander and
   dispatcher. Every one of those stages then re-ran on already-final text,
   so `command sed -e "s#a#A#"` lost its quoting, an argument containing a
   space became two, a literal `a*` re-globbed against the cwd, an embedded
   newline or `;` split the line into extra commands, an empty argument
   vanished -- and the re-dispatch found the shell function again, defeating
   the one thing `command` is for. run_external_sync takes the argv as it
   stands and never re-parses it. */
static int	command_run(t_shell *state, t_vec argv, size_t start)
{
	char	**av;
	t_vec	sub;
	size_t	i;
	int		rc;

	av = (char **)argv.ctx;
	vec_init(&sub);
	sub.elem_size = sizeof(char *);
	i = start;
	while (i < argv.len)
		vec_push(&sub, &av[i++]);
	if (builtin_func(av[start]))
		return (rc = builtin_func(av[start])(state, sub), xfree(sub.ctx), rc);
	rc = run_external_sync(state, &sub);
	return (xfree(sub.ctx), rc);
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
	if (start < argv.len && (!ft_strcmp(av[start], "-v")
			|| !ft_strcmp(av[start], "-V")))
		return (command_describe(state, argv, start + 1,
				av[start][1] == 'V'));
	if (start >= argv.len)
		return (0);
	return (command_run(state, argv, start));
}
