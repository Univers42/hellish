/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   assignment_shape.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/14 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* A plain NAME=value, written over a NAME that already holds an array.

   bash writes element 0 and keeps the rest: `x=(a b); x=c` is
   ([0]="c" [1]="b"), `x+=c` is ([0]="ac" [1]="b"), and for a map the
   element is the key "0". hellish replaced the whole array with the
   string.

   That is how bash-preexec lost its hook. It appends `__bp_install "$_"`
   to PROMPT_COMMAND as element 1 and waits for the first prompt to run
   it; z.sh, sourced next, does PROMPT_COMMAND="$PROMPT_COMMAND"$'\n'"(_z
   --add ...)" -- element 0 read, element 0 written, under bash. Under
   hellish that assignment took the array with it, the install string was
   gone before any prompt ran, and preexec/precmd were never wired up. */

/* The string a += appends to: element 0 of an array, the key "0" of a
   map, the value itself otherwise. Heap, and "" when there is none. */
char	*append_base(const char *old)
{
	char	*e;

	if (arr_is(old))
		e = arr_get_idx(old, 0);
	else if (assoc_is(old))
		e = assoc_get(old, "0", 1);
	else if (old)
		e = ft_strdup(old);
	else
		e = NULL;
	if (!e)
		e = ft_strdup("");
	return (e);
}

/* Rewrite ret->value as the array it must stay, when NAME holds one. A
   subscripted key is not ours; subscript_assign owns those. */
void	keep_array_shape(t_shell *state, t_env *ret)
{
	char	*old;
	char	*res;

	if (!ret->value || ft_strchr(ret->key, '['))
		return ;
	old = env_expand(state, ret->key);
	if (arr_is(old))
		res = arr_with_set(old, 0, ret->value);
	else if (assoc_is(old))
		res = assoc_with_set(old, "0", 1, ret->value);
	else
		return ;
	xfree(ret->value);
	ret->value = res;
}

/* Store ev, keeping the array shape of what it replaces. */
void	env_set_shaped(t_shell *state, t_env ev)
{
	keep_array_shape(state, &ev);
	env_set(&state->env, ev);
}
