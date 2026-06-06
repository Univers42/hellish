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
char	*lineno_str(t_shell *state);

/* If key[0..len) is all-digit > 0, return it; else -1.
** "0" falls through (so $0 stays an ordinary env entry). */
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

static char	*expand_special(t_shell *state, char *key, int len)
{
	if (ft_strncmp(key, "?", len) == 0 && len == 1)
		return (state->last_cmd_st);
	if (ft_strncmp(key, "$", len) == 0 && state->pid && len == 1)
		return (state->pid);
	if (key[0] == '!' && len == 1)
	{
		if (state->last_bg_pid)
			return (state->last_bg_pid);
		return ("");
	}
	if (len == 1 && key[0] == '-')
		return (build_flagstr(state));
	if (len == 6 && ft_strncmp(key, "LINENO", 6) == 0)
		return (lineno_str(state));
	if (len == 0)
		return ("");
	if (len == 1 && key[0] == '#')
	{
		if (state->pos.cnt_str[0])
			return (state->pos.cnt_str);
		return ("0");
	}
	return (NULL);
}

char	*env_expand_n(t_shell *state, char *key, int len)
{
	t_env	*curr;
	char	*sp;
	int		n;

	sp = expand_special(state, key, len);
	if (sp != NULL)
		return (sp);
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
	xfree(src->ctx);
	vec_init(src);
}
