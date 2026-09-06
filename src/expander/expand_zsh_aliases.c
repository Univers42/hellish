/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_aliases.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sh_alias.h"

/* zsh's `aliases` is a special associative parameter: `aliases[gco]='git
** checkout'` defines the alias, the way `alias gco='git checkout'` does.
** oh-my-zsh's git plugin writes its deprecation shims through it --
** `aliases[$old_name]="print -Pu2 ...; $new_name"` in a loop -- and as
** an ordinary array that assignment named no element and stopped the
** file with "aliases: assignment to invalid subscript range" at every
** shell start (issue #114).  In the dialect the write goes to the alias
** table.  Only the dialect: in bash `aliases[x]=y` is an array element
** like any other, and the golden suite pins that.
**
** The variable itself is left as it was.  The two callers go on to store
** `ev` in the environment whatever happens, so its value is rewritten to
** what `aliases` already held (usually nothing): the alias table changed,
** the variable did not.
*/

/* True when this is the dialect's aliases[...]: the alias is defined
   (an empty or unexpandable key defines nothing, as zsh refuses it). */
static bool	aliases_define(t_shell *state, t_env *ev, const char *sub,
				const char *value)
{
	const char	*rb;
	char		*key;

	if (!zsh_mode(state) || ft_strcmp(ev->key, "aliases"))
		return (false);
	rb = ft_strrchr((char *)sub, ']');
	if (!rb)
		return (false);
	key = expand_param_word(state, sub, (int)(rb - sub), false);
	if (key && *key && value)
		alias_set(&state->aliases, key, value);
	xfree(key);
	return (true);
}

/* Owned copy of what the variable `aliases` holds, "" when nothing. */
static char	*aliases_keep(t_shell *state, t_env *ev)
{
	char	*old;

	old = env_expand(state, ev->key);
	if (old)
		return (ft_strdup(old));
	return (ft_strdup(""));
}

/* The list form, `aliases[k]=( v )`, from splice_elem_assign: ev->value
   is not yet owned there, so it is only written. */
bool	zsh_aliases_assign(t_shell *state, t_env *ev, const char *sub,
			t_vec *args)
{
	const char	*value;

	value = NULL;
	if (args->len > 0)
		value = ((char **)args->ctx)[0];
	if (!aliases_define(state, ev, sub, value))
		return (false);
	ev->value = aliases_keep(state, ev);
	return (true);
}

/* The scalar form, `aliases[k]=v`, before subscript_assign gets the
   key: it still carries its "[k]" here.  The key is cut back to the bare
   name and the owned value replaced, so what reaches the environment is
   the variable as it was. */
bool	zsh_aliases_scalar(t_shell *state, t_env *ret)
{
	char	*br;

	if (!zsh_mode(state) || !ret->key || !ret->value)
		return (false);
	br = ft_strchr(ret->key, '[');
	if (!br || ft_strncmp(ret->key, "aliases[", 8))
		return (false);
	*br = '\0';
	if (!aliases_define(state, ret, br + 1, ret->value))
		return (*br = '[', false);
	xfree(ret->value);
	ret->value = aliases_keep(state, ret);
	return (true);
}
