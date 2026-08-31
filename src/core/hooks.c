/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   hooks.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 13:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 13:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "env.h"
#include "helpers.h"

int	exec_string(t_shell *state, char *content);

/* HELLISH_PRECMD_FUNCS and HELLISH_PREEXEC_FUNCS -- #72 phase 2.3.
**
** Two attachment points a plugin can use without taking them away from
** every other plugin. That is the entire design constraint, and it is why
** these are ARRAYS OF FUNCTION NAMES and not strings of code:
**
**     trap 'x' DEBUG      one handler. The second plugin to install one
**                         silently removes the first, and neither can tell.
**     PROMPT_COMMAND      one string. Appending to it is string surgery on
**                         something another plugin may also be appending to.
**     FUNCS=(a b)         two names, both run, order visible from `echo`.
**
** A plain space-separated string is accepted as well, because that is what
** someone writes before reading anything.
**
** Interactive shells only, the same rule ~/.hellishrc has always had: a
** script that inherits the developer's hooks is a test suite that fails on
** one machine.
*/

/* Call one hook by name, with the command line as $1 for preexec. The name
   is quoted through the shell rather than invoked directly so that a hook
   is an ordinary command -- it can be a function, an alias, or a builtin,
   and it reports its status the same way any of them would. */
void	hook_run_one(t_shell *state, const char *fn, const char *arg)
{
	t_string	cmd;
	char		*q;

	vec_init(&cmd);
	cmd.elem_size = 1;
	vec_push_str(&cmd, (char *)fn);
	if (arg)
	{
		q = sq_quote(arg);
		vec_push_char(&cmd, ' ');
		vec_push_str(&cmd, q);
		xfree(q);
	}
	vec_push_char(&cmd, '\0');
	exec_string(state, (char *)cmd.ctx);
	xfree(cmd.ctx);
}

/* The string spelling: HELLISH_PRECMD_FUNCS='a b'. */
static void	hook_split(t_shell *state, const char *val, const char *arg)
{
	char	**w;
	int		i;

	w = ft_split((char *)val, ' ');
	if (!w)
		return ;
	i = 0;
	while (w[i])
		hook_run_one(state, w[i++], arg);
	free_tab(w);
}

/* Walk the names, array or string. */
static void	hook_each(t_shell *state, const char *val, const char *arg)
{
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;
	char		*e;

	if (!arr_is(val))
		return (hook_split(state, val, arg));
	cur = val + 1;
	idx = 0;
	while (arr_next(&cur, &idx, &v, &vl))
	{
		e = ft_strndup((char *)v, (size_t)vl);
		hook_run_one(state, e, arg);
		xfree(e);
	}
}

/* Run every hook named by VAR, then put $? back.
**
** The copy of the value is load-bearing twice over. env_expand hands back a
** pointer INTO the environment table, and a hook is a shell command: one
** that assigns to any variable can grow the table, move it, and leave the
** walk reading freed memory -- and a hook that edits its OWN list, which is
** how a plugin unregisters itself, does it every time.
**
** Restoring the status is the other half. Without it the hook's own result
** becomes $?, so a PS1 with a status badge reports the hook forever -- the
** same frozen value #69 was reported for. PROMPT_COMMAND brackets this the
** same way in open_cycle.
*/
void	run_hook_funcs(t_shell *state, char *var, const char *arg)
{
	t_execution_state	saved;
	char				*val;

	val = env_expand(state, var);
	if (!val || !*val)
		return ;
	val = ft_strdup(val);
	if (!val)
		return ;
	saved = state->last_cmd_st_exe;
	hook_each(state, val, arg);
	set_cmd_status(state, saved);
	xfree(val);
}
