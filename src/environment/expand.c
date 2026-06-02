/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand.c                                           :+:      :+:    :+:   */
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
char	*build_flagstr(t_shell *state);

/* If key[0..len) is an all-digit positive number, return it; else -1. "0" and
** anything non-numeric fall through (so $0 stays an ordinary env entry). */
static int	pos_index(const char *key, int len)
{
	int	n;
	int	i;

	if (len <= 0)
		return (-1);
	n = 0;
	i = 0;
	while (i < len)
	{
		if (key[i] < '0' || key[i] > '9')
			return (-1);
		n = n * 10 + (key[i] - '0');
		i++;
	}
	if (n < 1)
		return (-1);
	return (n);
}

char	*env_expand_n(t_shell *state, char *key, int len)
{
	t_env	*curr;
	int		n;

	if (ft_strncmp(key, "?", len) == 0 && len == 1)
		return (state->last_cmd_st);
	else if (ft_strncmp(key, "$", len) == 0 && state->pid && len == 1)
		return (state->pid);
	else if (key[0] == '!' && len == 1)
		return (state->last_bg_pid ? state->last_bg_pid : "");
	else if (len == 1 && key[0] == '-')
		return (build_flagstr(state));
	else if (len == 0)
		return ("");
	else if (len == 1 && key[0] == '#')
		return (state->pos.cnt_str[0] ? state->pos.cnt_str : "0");
	n = pos_index(key, len);
	if (n >= 1)
	{
		if (n <= state->pos.count)
			return (state->pos.args[n - 1]);
		return (0);
	}
	curr = env_nget(&state->env, key, len);
	if (curr == 0 || curr->key == 0)
		return (0);
	return (curr->value);
}

char	*env_expand(t_shell *state, char *key)
{
	return (env_expand_n(state, key, ft_strlen(key)));
}

void	env_extend(t_vec_env *dest, t_vec_env *src, bool export)
{
	t_env	curr;

	while (src->len)
	{
		curr = *(t_env *)vec_pop(src);
		if (!curr.key)
			continue ;
		curr.exported = export;
		env_set(dest, curr);
	}
	free(src->ctx);
	vec_init(src);
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
		if (!el->key)
		{
			i++;
			continue ;
		}
		if (is_readonly_var(state, el->key))
		{
			ft_eprintf("%s: %s: readonly variable\n", state->ctx, el->key);
			free(el->key);
			free(el->value);
			el->key = NULL;
			el->value = NULL;
			if (state->metinp != INP_RL)
				exit_clean(state, 127);
			state->last_cmd_st_exe = (t_execution_state){.status = 1};
			i++;
			continue ;
		}
		el->exported = export;
		env_set(&state->env, *el);
		el->key = NULL;
		el->value = NULL;
		i++;
	}
}
