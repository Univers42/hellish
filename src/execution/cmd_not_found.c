/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cmd_not_found.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 15:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* bash's `command_not_found_handle`: a function the shell calls INSTEAD of
** printing "command not found", with the failed command and its arguments.
**
** It is not decoration. Debian and Ubuntu define it in /etc/bash.bashrc, and
** it is the entire reason a missing command answers
**
**     Command 'vim' not found, but can be installed with:
**     sudo apt install vim
**
** in bash while hellish said only "vim: command not found". A user comparing
** the two reads that as hellish failing to find a program bash can see --
** which is exactly how it was reported (issue #76). Both shells had in fact
** agreed the command did not exist; only one of them was helpful about it.
**
** `bash --posix` still calls it, so this is not gated on the dialect.
**
** The semantics below were measured against bash, not assumed:
**   * a name containing '/' is a path, not a PATH lookup -- no handler;
**   * found-but-not-executable stays 126 and does NOT reach the handler;
**   * the handler's exit status becomes the command's status;
**   * `command missing` consults it too;
**   * side effects do NOT persist (bash's own handler cannot cd the shell),
**     which is why running it here -- in the forked child, where hellish
**     discovers the miss -- matches bash rather than merely being convenient.
*/

/* Build the argv the handler is called with. Slot 0 is the handler's own
** name because execute_func_call() borrows argv+1 as $1.., so the command
** the user typed has to land in slot 1 to become $1 the way bash passes it.
**   Every element is BORROWED from `args`; only the vector backing belongs
** to us, so the caller frees that and nothing else.
*/
static bool	cnf_build_argv(t_vec *args, t_vec *out)
{
	char	*name;
	size_t	i;

	if (!vec_init(out))
		return (false);
	out->elem_size = sizeof(char *);
	name = CNF_HANDLER;
	if (!vec_push(out, &name))
		return (false);
	i = 0;
	while (i < args->len)
	{
		if (!vec_push(out, (char **)args->ctx + i))
			return (false);
		i++;
	}
	return (true);
}

/* -1 means "not handled, carry on with the normal lookup and its normal
** diagnostic". Anything >= 0 is the exit status the handler produced.
**   The PATH probe is repeated here rather than reusing the caller's result
** because the caller's version PRINTS on failure, and bash emits nothing at
** all when a handler exists. A second exe_path() walk costs one miss on a
** command that was about to fail anyway.
*/
int	try_cmd_not_found_handler(t_shell *state, t_vec *args)
{
	t_shell_func	*fn;
	char			*probe;
	t_vec			hargv;
	int				perm;
	int				st;

	if (!args->len || !args->ctx || ft_strchr(((char **)args->ctx)[0], '/'))
		return (-1);
	fn = func_lookup(state, CNF_HANDLER);
	if (!fn)
		return (-1);
	perm = 0;
	path_cache_sync(state);
	probe = exe_path(state->path_dirs, ((char **)args->ctx)[0], &perm);
	if (probe)
		return (xfree(probe), -1);
	if (perm)
		return (-1);
	if (!cnf_build_argv(args, &hargv))
		return (xfree(hargv.ctx), -1);
	st = execute_func_call(state, fn, &hargv).status;
	return (xfree(hargv.ctx), st);
}
