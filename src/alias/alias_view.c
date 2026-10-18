/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias_view.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/24 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sh_alias.h"
#include "shell.h"
#include "env.h"

/* zsh's `aliases` READ side (issue #137). The write side already reached
** the alias table (expand_zsh_aliases.c, #114); a read saw the plain,
** empty variable, so `${aliases[ll]}` was "" and `${(k)aliases}` listed
** nothing. oh-my-zsh's sudo widget -- bound to ESC ESC, on by default --
** asks `${${(Az)aliases[$cmd]}[1]:-$cmd}` for the command an alias runs.
**
** In the dialect the parameter IS the table: a read builds the table as
** an associative value, so every reader that already understands one --
** a subscript, (k) and (v), ${#...}, the flags -- works unchanged. The
** value is returned borrowed, like any variable's, from one of two
** state-owned slots: the one before is still valid, so two reads alive at
** once (`${aliases[a]}${aliases[b]}`) cannot see each other's free. */

static void	view_push(t_string *out, const t_alias_entry *a)
{
	if (out->len > 1)
		vec_push_char(out, ARR_RS);
	vec_push_str(out, a->name);
	vec_push_char(out, ARR_US);
	vec_push_str(out, a->value);
}

char	*zsh_aliases_view(t_shell *state)
{
	t_hash_entry	*e;
	t_string		out;
	size_t			i;
	int				slot;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, ARR_ASSOC_MAGIC);
	e = (t_hash_entry *)state->aliases.ctx;
	i = 0;
	while (e && i < state->aliases.cap)
	{
		if (e[i].key && e[i].value)
			view_push(&out, (t_alias_entry *)e[i].value);
		i++;
	}
	vec_push_char(&out, '\0');
	slot = state->aliases_view_at ^ 1;
	xfree(state->aliases_view[slot]);
	state->aliases_view[slot] = (char *)out.ctx;
	state->aliases_view_at = slot;
	return (state->aliases_view[slot]);
}
