/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_simple_command2.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:52 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/09 23:30:52 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Alias expansion no longer happens here: it moved to the input scanner
   (src/alias/alias_scan.c), which runs before the lexer so quoting,
   keywords in alias bodies, recursion and the trailing-blank rule all
   behave like bash.  Exec-time argv splicing could honor none of those. */

/* After glob/IFS expansion some argv slots can be NULL or a low pointer
   (sentinel values from the slab allocator).  execve(2) would crash or
   behave oddly if it saw those, so we replace any such slot with a fresh
   empty string.  The uintptr_t < 4096 guard catches slab sentinels that
   are not literally NULL but still invalid as C strings. */
void	replace_null_argv_with_empty(t_executable_cmd *cmd)
{
	size_t	i;
	char	*p;

	i = 0;
	while (i < cmd->argv.len)
	{
		p = ((char **)cmd->argv.ctx)[i];
		if (p == NULL || (uintptr_t)p < 4096)
			((char **)cmd->argv.ctx)[i] = ft_strdup("");
		i++;
	}
}

/* Restore the three standard fds from a bak[3] produced by dup().  Used
   by callers that saved before redirecting (exec_builtin, func_call).
   Must be paired with take_backup_fds and called even on error paths to
   avoid leaking the dup'd fds. */
void	restore_fds(int *bak)
{
	dup2(bak[0], 0);
	dup2(bak[1], 1);
	dup2(bak[2], 2);
	close(bak[0]);
	close(bak[1]);
	close(bak[2]);
}

/* The POSIX special builtins.  Their defining property here is not how
   they are dispatched -- hellish already runs them in the parent -- but
   what happens when one of them FAILS: POSIX says an error in a special
   builtin shall abort a non-interactive shell, where the same error in
   any other utility only sets $?.  bash implements exactly that:

    $ bash --posix -c 'exec 3</nonexistent; echo after'   -> no "after", rc 1
    $ bash --posix -c 'true 3</nonexistent; echo after'   -> "after",    rc 0
*/
static bool	is_special_builtin(const char *name)
{
	const char	*tab;
	char		pat[24];

	tab = " : . break continue eval exec exit export readonly return set"
		" shift times trap unset ";
	if (!name || ft_strlen(name) > 16)
		return (false);
	pat[0] = ' ';
	ft_strlcpy(pat + 1, name, sizeof(pat) - 2);
	ft_strlcat(pat, " ", sizeof(pat));
	return (ft_strnstr(tab, pat, ft_strlen(tab)) != NULL);
}

/* Decide whether a redirection or expansion error that just killed this
   command must take the whole shell down with it.  Only for a special
   builtin, and only when the shell is not interactive -- an interactive
   user gets their prompt back, as bash does.  The caller reads this
   BEFORE freeing cmd, then exits after the teardown so nothing leaks. */
bool	redir_err_is_fatal(t_shell *state, t_executable_cmd *cmd)
{
	if (state->metinp == INP_RL)
		return (false);
	if (!cmd->argv.ctx || cmd->argv.len == 0)
		return (false);
	if (!((char **)cmd->argv.ctx)[0])
		return (false);
	return (is_special_builtin(((char **)cmd->argv.ctx)[0]));
}

/* The other half of the special-builtin abort rule: a special builtin
   that fails on its OPERANDS, not on a redirection.  Status alone cannot
   decide it -- `shift 99` and `trap "" BADSIG` both return 1 in bash
   without aborting, because that is a result, not an error -- so this is
   restricted to the three whose non-zero return always means the request
   was malformed.  Measured against bash 5.3.9:

    export 1BAD / export -Z / readonly 1BAD / readonly -Z / unset -Z
    and `readonly R=1; unset R`  all abort;  `unset 1BAD` returns 0. */
bool	strict_builtin_failed(t_shell *state, t_executable_cmd *cmd,
			int status)
{
	const char	*name;

	if (status == 0 || state->metinp == INP_RL)
		return (false);
	if (!cmd->argv.ctx || cmd->argv.len == 0)
		return (false);
	name = ((char **)cmd->argv.ctx)[0];
	if (!name)
		return (false);
	return (ft_strcmp(name, "export") == 0 || ft_strcmp(name, "readonly") == 0
		|| ft_strcmp(name, "unset") == 0);
}
