/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   func_scope.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

void	try_unset(t_shell *state, char *key);

/* Remember `key`'s current value so it can be restored when the current
   function returns (used for `local` and for the positional params $1..). */
void	scope_save(t_shell *state, const char *key)
{
	t_scope_save	s;
	t_env			*e;

	if (state->local_saves.elem_size == 0)
	{
		vec_init(&state->local_saves);
		state->local_saves.elem_size = sizeof(t_scope_save);
	}
	s.depth = state->func_depth;
	s.key = ft_strdup(key);
	e = env_get(&state->env, (char *)key);
	s.existed = (e != NULL);
	if (e && e->value)
		s.value = ft_strdup(e->value);
	else
		s.value = NULL;
	vec_push(&state->local_saves, &s);
}

static void	restore_one(t_shell *state, t_scope_save *s)
{
	if (s->existed)
		env_set(&state->env, env_create(s->key,
				s->value ? s->value : ft_strdup(""), false));
	else
	{
		try_unset(state, s->key);
		free(s->key);
		free(s->value);
	}
}

/* Restore every variable saved at the current function depth (LIFO), then drop
   one depth level. Called when a function returns. */
void	scope_leave(t_shell *state)
{
	t_scope_save	*s;

	while (state->local_saves.len > 0)
	{
		s = (t_scope_save *)vec_idx(&state->local_saves,
				state->local_saves.len - 1);
		if (s->depth != state->func_depth)
			break ;
		restore_one(state, s);
		state->local_saves.len--;
	}
	state->func_depth--;
}

/* Apply NAME=val prefix assignments (cmd->pre_assigns) temporarily before a
   regular builtin or function runs, saving prior values so restore_temp_assigns
   can revert them (POSIX: such assignments don't persist past a regular builtin
   or function). Returns the save list (caller passes it to restore). */
t_vec	apply_temp_assigns(t_shell *state, t_vec *pre)
{
	t_vec			saves;
	t_scope_save	s;
	t_env			*pa;
	t_env			*e;
	size_t			i;

	vec_init(&saves);
	saves.elem_size = sizeof(t_scope_save);
	i = 0;
	while (i < pre->len)
	{
		pa = (t_env *)vec_idx(pre, i++);
		e = env_get(&state->env, pa->key);
		s.depth = 0;
		s.key = ft_strdup(pa->key);
		s.existed = (e != NULL);
		s.value = NULL;
		if (e && e->value)
			s.value = ft_strdup(e->value);
		vec_push(&saves, &s);
		env_set(&state->env, env_create(ft_strdup(pa->key),
				ft_strdup(pa->value ? pa->value : ""), false));
	}
	return (saves);
}

void	restore_temp_assigns(t_shell *state, t_vec *saves)
{
	size_t	i;

	i = saves->len;
	while (i > 0)
		restore_one(state, (t_scope_save *)vec_idx(saves, --i));
	free(saves->ctx);
}

/* local name[=value] ... : make each name local to the current function. */
int	builtin_local(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;
	char	*eq;
	char	*key;

	av = (char **)argv.ctx;
	if (state->func_depth <= 0)
		return (ft_eprintf("%s: local: can only be used in a function\n",
				state->ctx), 1);
	i = 1;
	while (i < argv.len)
	{
		eq = ft_strchr(av[i], '=');
		if (eq)
			key = ft_strndup(av[i], eq - av[i]);
		else
			key = ft_strdup(av[i]);
		scope_save(state, key);
		env_set(&state->env, env_create(key,
				ft_strdup(eq ? eq + 1 : ""), false));
		i++;
	}
	return (0);
}
