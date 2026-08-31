/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   hooks2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 13:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 13:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "helpers.h"
#include "sh_input.h"
#include "env.h"
#include "ft_builtins.h"

/* The preexec side: fired once per typed line, just before it runs, with
** the line itself as $1.
**
** The line is copied out of state->input by LENGTH rather than read as a C
** string. The input buffer is a t_string the reader appends into; treating
** it as NUL-terminated is true today and is exactly the kind of assumption
** that becomes false in a later refactor with no visible symptom until a
** hook receives trailing garbage.
**
** The trailing newline goes, because $1 is the command the user typed and
** a hook that echoes it should not produce a blank line.
*/
void	run_preexec(t_shell *state)
{
	char	*line;
	size_t	n;

	if (state->metinp != INP_RL || !state->input.ctx || !state->input.len)
		return ;
	n = state->input.len;
	while (n > 0 && (((char *)state->input.ctx)[n - 1] == '\n'
		|| ((char *)state->input.ctx)[n - 1] == '\0'))
		n--;
	if (n == 0)
		return ;
	line = ft_strndup((char *)state->input.ctx, n);
	if (!line)
		return ;
	run_hook_funcs(state, "HELLISH_PREEXEC_FUNCS", line);
	run_zsh_prompt_hooks(state, "preexec", line);
	xfree(line);
}

/* zsh's own hook convention -- issue #91. A function NAMED precmd or
** preexec runs at the corresponding moment, then every name registered in
** precmd_functions / preexec_functions (add-zsh-hook writes those). Every
** zsh prompt tutorial ends with `precmd() { vcs_info }`, and until this,
** that function was defined, recorded, and never called.
**
** The one deliberate absence: when bash-preexec is loaded, it OWNS the
** convention -- it appends the bare functions to its own arrays and fires
** everything through PROMPT_COMMAND. Firing here too would run every hook
** twice per prompt. Its import guard is the detection, both spellings. */
void	run_zsh_prompt_hooks(t_shell *state, const char *which,
			const char *arg)
{
	t_execution_state	saved;
	char				*probe;

	probe = env_expand(state, "bash_preexec_imported");
	if (probe && *probe)
		return ;
	probe = env_expand(state, "__bp_imported");
	if (probe && *probe)
		return ;
	if (func_lookup(state, (char *)which))
	{
		saved = state->last_cmd_st_exe;
		hook_run_one(state, which, arg);
		set_cmd_status(state, saved);
	}
	if (!ft_strcmp(which, "precmd"))
		run_hook_funcs(state, "precmd_functions", NULL);
	else
		run_hook_funcs(state, "preexec_functions", arg);
}
