/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   func_scope2.c                                      :+:      :+:    :+:   */
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

/* Fill a scope save for `key`: its current value AND its var_attrs entry.
**
** This exists because there are TWO places that build a t_scope_save --
** scope_save() for `local`, and save_and_apply_one() for a temporary
** NAME=val prefix -- and they were filling it field by field. Adding
** attr_kind/attr_target for `local -n` updated one of them, so the other
** pushed an uninitialised pointer that restore_one() then freed:
**
**     malloc: failed assertion: free: unallocated block
**
** It took sourcing git's own git-prompt.sh to surface it, and only in an
** optimised build -- the debug build left the stack slot benign. One
** constructor is the fix; a second construction site is the bug. */
void	scope_save_capture(t_shell *state, const char *key, t_scope_save *s)
{
	t_env	*e;

	s->key = ft_strdup(key);
	e = env_get(&state->env, (char *)key);
	s->existed = (e != NULL);
	s->value = NULL;
	if (e && e->value)
		s->value = ft_strdup(e->value);
	s->attr_kind = attr_kind(state, key, (int)ft_strlen(key));
	s->attr_target = NULL;
	if (attr_target(state, key, (int)ft_strlen(key)))
		s->attr_target = ft_strdup(attr_target(state, key,
					(int)ft_strlen(key)));
}

/* Roll back temporary NAME=val assignments after a builtin or function
   returns.  We iterate in reverse (LIFO) so nested saves unwind in the
   correct order.  The saves vec backing is freed with xfree after the
   loop because restore_one does NOT free the vec itself. */
void	restore_temp_assigns(t_shell *state, t_vec *saves)
{
	size_t	i;

	i = saves->len;
	while (i > 0)
		restore_one(state, (t_scope_save *)vec_idx(saves, --i));
	xfree(saves->ctx);
}

/* Write the initial value for a `local` variable.  If no '=' was given
   the variable is set to "" (not unset) so `${local_var:-default}` does
   not fall through to the default.  Under `set -a` (allexport) a valued
   `local NAME=v` is exported, exactly like bash and dash; a valueless
   `local NAME` stays unexported because bash leaves it unset — exporting
   our "" placeholder would put NAME= in the environment where bash shows
   nothing.  The key string is owned by the env entry after env_create;
   do not xfree it here. */
void	local_set_var(t_shell *state, char *key, char *eq)
{
	if (eq)
		env_set(&state->env,
			env_create(key, ft_strdup(eq + 1), state->opt_allexport));
	else
		env_set(&state->env, env_create(key, ft_strdup(""), false));
}

/* The saves vec itself, at shutdown.
**
** scope_leave restores each entry and drops the LENGTH; nothing ever
** released the backing buffer, so the first `local` in a session put ~80
** bytes on the ft_malloc oracle and left them there. Invisible to ASan --
** the vec is still reachable through t_shell -- and invisible to a
** zero-leak claim made from a run that never called a function.
**
** Any entries still present are a shell that exited from INSIDE a function
** (`f() { local v=1; exit 0; }`), so their scope never left. They are freed
** rather than restored: restoring writes into an environment that is about
** to be freed, and restore_one hands ownership of key/value to env_set,
** which would make this a double free instead of a fix.
*/
void	free_local_saves(t_shell *state)
{
	t_scope_save	*s;

	while (state->local_saves.len > 0)
	{
		s = (t_scope_save *)vec_idx(&state->local_saves,
				state->local_saves.len - 1);
		xfree(s->attr_target);
		xfree(s->key);
		xfree(s->value);
		state->local_saves.len--;
	}
	xfree(state->local_saves.ctx);
	state->local_saves.ctx = NULL;
	state->local_saves.elem_size = 0;
}
