/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cmd_not_found_hook.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "cmd_hash.h"

/* `command_not_found_handle` -- the hook bash runs INSTEAD of printing
** "command not found", with the failed command as "$@".
**
** It is what oh-my-zsh's `command-not-found` plugin, and every distro's
** "you can install it with apt install foo" helper, is built on.  Without
** it those plugins load, define the function, and are never called: the
** shell prints its own message and exits 127, so the hook looks broken
** rather than absent.
**
** Two properties are load-bearing:
**   - it runs in the PARENT.  The diagnostic it replaces is printed by the
**     forked child (exe_error.c), and a handler that ran there could not
**     cd, set a variable, or define an alias -- which is most of what real
**     handlers do.
**   - its EXIT STATUS is the command's.  bash does not force 127; a handler
**     that returns 42 makes `$?` 42, and one that succeeds makes it 0.
**     That falls out of routing through the ordinary function-call path.
*/

/* Would this command name reach "command not found"?  A name containing a
   slash is a path, and bash reports a missing path directly rather than
   calling the hook, so those are excluded.  Everything else is not-found
   exactly when the PATH scan that prehash_external just ran cached
   nothing for it. */
bool	cnf_not_found(t_shell *state, char *argv0)
{
	if (!argv0 || !*argv0 || ft_strchr(argv0, '/'))
		return (false);
	if (builtin_func(argv0) || func_lookup(state, argv0))
		return (false);
	return (cmd_hash_lookup(&state->cmd_cache, argv0) == NULL);
}

/* True when the hook exists AND this command would otherwise fail with
   "command not found".  Guarding on the hook first keeps the common path
   (no handler defined, which is every shell that has not loaded one) to a
   single hash lookup. */
bool	cnf_hook_applies(t_shell *state, char *argv0)
{
	if (!state->functions.len || !func_lookup(state, CNF_HOOK))
		return (false);
	return (cnf_not_found(state, argv0));
}

/* Make the failed command line the handler's ARGUMENTS: the hook's own name
   goes in front so the ordinary function-call path, which reads argv[0] as
   the callee and argv[1..] as "$@", hands the handler `$1` = the command
   that was not found -- bash's contract.
     The shift is in place: every original pointer keeps its single owner in
   cmd->argv and is still freed exactly once by word_free, which is why this
   does not copy the vector.  The name at [0] is an ordinary allocation and
   word_free routes it correctly (it decides slab vs xfree by address). */
bool	cnf_shift_argv(t_vec *argv)
{
	char	*hook;
	char	**a;
	size_t	i;

	hook = ft_strdup(CNF_HOOK);
	if (!hook)
		return (false);
	vec_push(argv, &hook);
	a = (char **)argv->ctx;
	i = argv->len - 1;
	while (i > 0)
	{
		a[i] = a[i - 1];
		i--;
	}
	a[0] = hook;
	return (true);
}
