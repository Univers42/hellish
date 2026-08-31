/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle_cwd.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 22:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 22:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "zle.h"
#include "ft_builtins.h"
#include "helpers.h"
#include <unistd.h>

/* Declared where it is defined (src/builtins), forward-declared here the way
   builtin_dirstack.c already does for the same function -- reusing the one
   routine that rotates PWD/OLDPWD rather than open-coding a second copy that
   could drift from cd's. */
void	update_pwd_vars(t_shell *state);

/* Carrying a widget's `cd` back across the readline fork -- #80 item 2.
**
** readline runs in a forked child (bg_readline), so a widget's edits to
** BUFFER survive -- the line is what the child sends back -- while anything
** else it changes does not. A widget that runs `cd` moved the CHILD and the
** parent never learned. That is why oh-my-zsh's `sudo`, which only rewrites
** the buffer, worked here while `dirhistory`, whose widgets navigate, did
** not: its keys fired, its widget ran, and the shell stayed put.
**
** #80 offered two ways out. Running widgets in the parent means not forking
** for readline at all, which the signal handling and the prompt animation
** both currently assume. The other was a round-trip protocol, with the
** caveat that it "changes what the pipe carries and every consumer of it".
**
** So the cwd travels BESIDE the line, on its own pipe, and the line protocol
** is not touched at all -- no consumer of it needs to know this exists.
**
** What is sent is the RESULT (a directory) rather than a command to re-run.
** A command would have to be re-executed in the parent, running the widget's
** side effects a second time; a path is idempotent, and adopting it is the
** same operation `cd` itself performs.
**
** Only a widget can make the two differ: readline does not chdir, and the
** child does nothing else between the fork and the report. So "the child
** ended somewhere else" means exactly "a widget moved it, deliberately".
*/

/* The pipe, parked where both halves of the fork can reach it without
   changing bg_readline's or attach_input_readline's signatures. Same
   function-local-static arrangement as zle_caller_cell. */
int	*zle_cwd_pipe(void)
{
	static int	fds[2] = {-1, -1};

	return (fds);
}

/* Open it before the fork. On failure the fds stay -1 and both halves below
   become no-ops: a shell that cannot report its cwd must still read a line,
   so this degrades to the old behaviour rather than failing the prompt. */
void	zle_cwd_open(void)
{
	int	*fds;

	fds = zle_cwd_pipe();
	fds[0] = -1;
	fds[1] = -1;
	if (pipe(fds) != 0)
	{
		fds[0] = -1;
		fds[1] = -1;
	}
}

/* Child side: report where we ended up, then close both ends.
   Called after readline() has returned, so any widget that ran has already
   done its work. */
void	zle_cwd_send(void)
{
	int		*fds;
	char	*cwd;

	fds = zle_cwd_pipe();
	if (fds[1] < 0)
		return ;
	close(fds[0]);
	fds[0] = -1;
	cwd = x_getcwd();
	if (cwd)
	{
		write_to_file(cwd, fds[1]);
		xfree(cwd);
	}
	close(fds[1]);
	fds[1] = -1;
}

/* Parent side: adopt the child's directory when it differs from ours.
**
** chdir first and only then update the cached cwd and PWD/OLDPWD, so a
** directory that has been removed since the widget ran leaves the shell
** exactly where it was instead of pointing PWD at somewhere it is not.
** run_chpwd_hooks fires here rather than in the child for the same reason
** the cd does: the child's hooks died with it, and a plugin that registers
** `add-zsh-hook chpwd` expects exactly one notification per move.
*/
static void	zle_cwd_apply(t_shell *state, char *path)
{
	char	*cwd;

	if (!state || !*path || chdir(path) != 0)
		return ;
	cwd = x_getcwd();
	if (cwd)
	{
		state->cwd.len = 0;
		vec_push_str(&state->cwd, cwd);
		xfree(cwd);
	}
	update_pwd_vars(state);
	run_chpwd_hooks(state);
}

/* Parent side: drain the report and act on it. Always closes the pipe, so a
   child that died before writing (^C in readline) costs one empty read and
   leaks no descriptor. */
void	zle_cwd_adopt(t_shell *state)
{
	int		*fds;
	char	buf[4096];
	ssize_t	n;

	fds = zle_cwd_pipe();
	if (fds[0] < 0)
		return ;
	close(fds[1]);
	fds[1] = -1;
	n = read(fds[0], buf, sizeof(buf) - 1);
	close(fds[0]);
	fds[0] = -1;
	if (n <= 0)
		return ;
	buf[n] = '\0';
	if (ft_strcmp(buf, (char *)state->cwd.ctx) != 0)
		zle_cwd_apply(state, buf);
}
