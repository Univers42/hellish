/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_command.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/14 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_builtins.h"
#include "shell.h"
#include "helpers.h"
#include "env.h"
#include "sh_input.h"
#include "job_control.h"
#include "update.h"
#include "sys.h"
#include "pal.h"
#include <stdlib.h>
#include <fcntl.h>
#include <locale.h>
#include <unistd.h>

/* $PROMPT_COMMAND, right before each interactive primary prompt.

   Since bash 5.1 it may be an array, and then each element is a command
   of its own, run in order: bash-preexec relies on that -- it appends
   `__bp_install "$_"` as a new element rather than editing a string it
   does not own, and after install PROMPT_COMMAND is
   ('__bp_precmd_invoke_cmd "$_"<newline>...' '__bp_interactive_mode').
   hellish handed the whole stored value to exec_string as one string, and
   for an array the stored value is the encoded record list, so what ran
   was `0_hx_precmd_run` and `1__bp_install`, each answered with "command
   not found" at every prompt.

   The value is copied before anything runs, for the reason run_hook_funcs
   gives: env_expand points into the table, and a hook that assigns --
   __bp_install rewrites PROMPT_COMMAND itself -- can move it. The status
   of the user's last command is put back afterwards, so the prompt's $?
   is theirs and not the hook's. */

static void	run_each(t_shell *state, const char *val)
{
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;
	char		*e;

	if (!arr_is(val))
	{
		exec_string(state, (char *)val);
		return ;
	}
	cur = val + 1;
	while (arr_next(&cur, &idx, &v, &vl))
	{
		e = ft_strndup((char *)v, (size_t)vl);
		if (e && *e)
			exec_string(state, e);
		xfree(e);
	}
}

void	run_prompt_command(t_shell *state)
{
	t_execution_state	saved;
	char				*pc;

	pc = env_expand(state, "PROMPT_COMMAND");
	if (!pc || !*pc)
		return ;
	pc = ft_strdup(pc);
	if (!pc)
		return ;
	saved = state->last_cmd_st_exe;
	run_each(state, pc);
	set_cmd_status(state, saved);
	xfree(pc);
}
