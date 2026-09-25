/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bind_rl2.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"

/* The two names `bind` checks when it is called, not when readline gets
   the request: readline answers both from static tables, before it has
   been initialised. */
bool	bind_rl_keymap_ok(const char *name)
{
	return (rl_get_keymap_by_name(name) != NULL);
}

bool	bind_rl_fn_ok(const char *name)
{
	return (rl_named_function(name) != NULL);
}

/* What one request does, in whatever keymap is current. A readline line
   goes through rl_parse_and_bind, which edits its argument: it gets a
   copy. */
static void	bind_line_do(t_bind_line *b)
{
	t_zle_fn	f;
	char		*copy;

	if (b->op == 'u')
	{
		f = rl_named_function(b->line);
		if (f)
			rl_unbind_function_in_map(f, rl_get_keymap());
	}
	else if (b->op == 'r')
		rl_bind_keyseq(b->line, NULL);
	else
	{
		copy = ft_strdup(b->line);
		if (copy)
			rl_parse_and_bind(copy);
		xfree(copy);
	}
}

/* One recorded request, in the keymap it named (-m), then the keymap
   that was current before it is put back. */
void	bind_line_apply(t_bind_line *b)
{
	Keymap	saved;
	Keymap	map;

	saved = rl_get_keymap();
	map = NULL;
	if (b->map)
		map = rl_get_keymap_by_name(b->map);
	if (map)
		rl_set_keymap(map);
	bind_line_do(b);
	rl_set_keymap(saved);
}
