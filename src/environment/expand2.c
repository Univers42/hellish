/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:02:38 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:02:38 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "shell.h"
#include "helpers.h"
#include "sh_input.h"

bool	is_readonly_var(t_shell *state, const char *key);
void	exit_clean(t_shell *state, int code);

/* $LINENO : the line number currently being read/executed (input-line
   granularity, like bash for multiple commands on one source line). */
char	*lineno_str(t_shell *state)
{
	char	*s;

	s = ft_itoa(state->rl.line);
	if (!s)
		return (ft_strlcpy(state->linebuf, "0", 2), state->linebuf);
	ft_strlcpy(state->linebuf, s, sizeof(state->linebuf));
	xfree(s);
	return (state->linebuf);
}

static void	handle_readonly_assign(t_shell *state, t_env *el)
{
	ft_eprintf("%s: %s: readonly variable\n", state->ctx, el->key);
	xfree(el->key);
	xfree(el->value);
	el->key = NULL;
	el->value = NULL;
	if (state->metinp != INP_RL)
		exit_clean(state, 127);
	state->last_cmd_st_exe = (t_execution_state){.status = 1};
}

static void	apply_one_assign(t_shell *state, t_env *el, bool export)
{
	if (is_readonly_var(state, el->key))
	{
		handle_readonly_assign(state, el);
		return ;
	}
	el->exported = export;
	env_set(&state->env, *el);
	el->key = NULL;
	el->value = NULL;
}

void	env_apply_cmd_assigns(t_shell *state,
			t_executable_cmd *src, bool export)
{
	size_t	i;
	t_env	*el;

	if (!state || !src)
		return ;
	i = 0;
	while (i < src->pre_assigns.len)
	{
		el = &((t_env *)src->pre_assigns.ctx)[i];
		if (el->key)
			apply_one_assign(state, el, export);
		i++;
	}
}
