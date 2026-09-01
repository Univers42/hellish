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

#include "env.h"
#include "execution_private.h"
#include "ft_builtins.h"

int		try_unset(t_shell *state, char *key);
void	unset_raw(t_shell *state, char *key);

/* Remember `key`'s current value so it can be restored when the current
   function returns (used for `local` and for the positional params $1..). */
void	scope_save(t_shell *state, const char *key)
{
	t_scope_save	s;

	if (state->local_saves.elem_size == 0)
	{
		vec_init(&state->local_saves);
		state->local_saves.elem_size = sizeof(t_scope_save);
	}
	s.depth = state->func_depth;
	scope_save_capture(state, key, &s);
	vec_push(&state->local_saves, &s);
}

/* Restore one saved variable to its pre-function value.  If it existed
   before the function was called, put it back; if it did not exist,
   unset it (try_unset) and free the key/value strings.  The env_create
   path always strdup's value, so we pass s->value directly -- env_set
   takes ownership and we must not xfree it afterward. */
void	restore_one(t_shell *state, t_scope_save *s)
{
	attr_set(state, s->key, s->attr_kind, s->attr_target);
	xfree(s->attr_target);
	s->attr_target = NULL;
	if (s->existed)
	{
		if (!s->value)
			s->value = ft_strdup("");
		env_set(&state->env, env_create(s->key, s->value, false));
	}
	else
	{
		unset_raw(state, s->key);
		xfree(s->key);
		xfree(s->value);
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

/* Save the current value of pa->key then apply the temporary assignment.
   depth=0 marks this as a "temporary assign" save (as opposed to a `local`
   save at func_depth>0) so restore_temp_assigns can iterate the same saves
   vec without confusing the two kinds of saves. */
static void	save_and_apply_one(t_shell *state, t_vec *saves, t_env *pa)
{
	t_scope_save	s;

	s.depth = 0;
	scope_save_capture(state, pa->key, &s);
	vec_push(saves, &s);
	if (pa->value)
		env_set(&state->env,
			env_create(ft_strdup(pa->key), ft_strdup(pa->value), false));
	else
		env_set(&state->env,
			env_create(ft_strdup(pa->key), ft_strdup(""), false));
}

/* Apply NAME=val assignments temporarily; save prior values for restore. */
t_vec	apply_temp_assigns(t_shell *state, t_vec *pre)
{
	t_vec	saves;
	size_t	i;

	vec_init(&saves);
	saves.elem_size = sizeof(t_scope_save);
	i = 0;
	while (i < pre->len)
		save_and_apply_one(state, &saves, (t_env *)vec_idx(pre, i++));
	return (saves);
}
