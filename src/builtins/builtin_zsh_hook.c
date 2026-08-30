/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_hook.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 03:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 03:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"

int			exec_string(t_shell *state, char *content);

/* True while a chpwd hook is running.  File-scope rather than a static
   local only to stay inside the norm's five declarations per function. */
static bool	g_chpwd_running = false;

/* `add-zsh-hook chpwd fn` -- run `fn` after every directory change.
**
**     autoload -U add-zsh-hook
**     add-zsh-hook chpwd chpwd_dirhistory
**
** is oh-my-zsh's dirhistory, and without it the plugin loads and then does
** nothing at all: chpwd_dirhistory is the ONLY caller of its push_past, so
** an unhooked dirhistory has a permanently empty history and every ALT-LEFT
** decides someone overwrote its variable.
**
** The registry is an array named `<hook>_functions`, which is not an
** implementation choice -- it is zsh's documented interface, and plugins
** append to it directly (`precmd_functions+=(my_fn)`) as often as they call
** this builtin.  Storing it anywhere else would make the two spellings
** disagree.
**
** ONLY `chpwd` FIRES.  precmd/preexec/periodic/zshaddhistory are recorded
** the same way and never run, because their firing points are the prompt
** loop and hellish already has PROMPT_COMMAND and the DEBUG trap there --
** bash-preexec, in this same corpus, is built on exactly those.  Adding a
** second, differently-timed set of prompt callbacks would mean two answers
** to "when does this run".  Registering a hook that cannot fire is reported
** rather than accepted silently, so a plugin relying on one is not left
** looking installed.
*/

/* Is `fn` already in the encoded array `arr`?  zsh's add-zsh-hook does not
   duplicate, and a plugin sourced twice must not run its hook twice. */
static bool	hook_has(const char *arr, const char *fn)
{
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;

	if (!arr_is(arr))
		return (false);
	cur = arr + 1;
	while (arr_next(&cur, &idx, &v, &vl))
	{
		if (vl == (int)ft_strlen(fn) && !ft_strncmp(v, fn, (size_t)vl))
			return (true);
	}
	return (false);
}

/* Hook names hellish has a firing point for.  The others are accepted into
   the array (so `${precmd_functions[@]}` still reads back what was put in)
   but reported, because nothing will call them. */
static int	hook_warn_unfired(t_shell *state, const char *hook)
{
	if (!ft_strcmp(hook, "chpwd"))
		return (0);
	ft_eprintf("%s: add-zsh-hook: %s is recorded but never fires; "
		"use PROMPT_COMMAND or trap DEBUG\n", state->ctx, hook);
	return (1);
}

/* add-zsh-hook [-Uzk] hook function.  The autoload-ish flags are accepted
   and ignored -- they say how zsh should LOAD the function, which is moot
   once it is already defined.  `-d`/`-D` remove a hook; nothing in the
   corpus uses them, and answering 0 for a removal that did not happen is
   the failure mode this whole file exists to avoid, so they are refused. */
int	builtin_add_zsh_hook(t_shell *state, t_vec argv)
{
	char	**av;
	char	*name;
	size_t	i;
	int		rc;

	av = (char **)argv.ctx;
	i = 1;
	while (i < argv.len && av[i][0] == '-' && av[i][1])
	{
		if (ft_strchr(av[i], 'd') || ft_strchr(av[i], 'D'))
			return (ft_eprintf("%s: add-zsh-hook: %s: not supported\n",
					state->ctx, av[i]), 2);
		i++;
	}
	if (i + 1 >= argv.len)
		return (ft_eprintf("%s: add-zsh-hook: expected hook and function\n",
				state->ctx), 2);
	rc = hook_warn_unfired(state, av[i]);
	name = ft_strjoin(av[i], "_functions");
	if (!hook_has(env_expand(state, name), av[i + 1]))
		env_set(&state->env, env_create(ft_strdup(name),
				arr_from_elems(&av[i + 1], 1,
					env_expand(state, name)), false));
	return (xfree(name), rc);
}

/* Run every function in chpwd_functions, in order, after a successful cd.
**
** The guard is not defensive coding: dirhistory's own widgets cd, and a
** chpwd hook that cds (directly, or through a helper it calls) would
** re-enter this from inside itself and recurse until the stack ran out.
** zsh guards the same way.
**
** $? is preserved across the hooks, because `cd somewhere && echo ok`
** must not start reporting the exit status of a plugin's bookkeeping.
**
** exec_string does NOT take the string: it alias-expands into a copy and
** frees only that, so the caller still owns what it passed.
*/
void	run_chpwd_hooks(t_shell *state)
{
	t_execution_state	saved;
	char				*fn;
	long				n;
	long				i;

	n = arr_count(env_expand(state, "chpwd_functions"));
	if (g_chpwd_running || n <= 0)
		return ;
	g_chpwd_running = true;
	saved = state->last_cmd_st_exe;
	i = -1;
	while (++i < n)
	{
		fn = arr_get_idx(env_expand(state, "chpwd_functions"), i);
		if (fn)
			exec_string(state, fn);
		xfree(fn);
	}
	set_cmd_status(state, saved);
	g_chpwd_running = false;
}
