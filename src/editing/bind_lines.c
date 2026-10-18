/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bind_lines.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "zle.h"

/* What `bind` asked readline to do, kept until readline can take it.
**
** The same timing problem `bindkey` has (zle_bind.c): an rc file runs
** before readline is initialised and before the editing mode installs the
** keymap a binding would land in, which replaces whatever was there. So
** each request is recorded here and replayed, in order, right before a
** line is read (rl_preinit) -- and replayed again from the start whenever
** the editing mode changes, since the new keymap is empty of it.
**
** One entry per request: `op` is 0 for a readline line
** (`"\e[A": history-search-backward`, `set completion-ignore-case on`,
** `$include FILE` for -f), 'u' to unbind a function, 'r' to unbind a key
** sequence. `map` is -m's keymap, NULL for the current one. */

t_vec	*bind_lines(void)
{
	static t_vec	v;

	if (!v.elem_size)
	{
		vec_init(&v);
		v.elem_size = sizeof(t_bind_line);
	}
	return (&v);
}

void	bind_line_add(char op, const char *map, const char *line)
{
	t_bind_line	b;

	b.op = op;
	b.map = NULL;
	if (map)
		b.map = ft_strdup(map);
	b.line = ft_strdup(line);
	if (!b.line)
		return (xfree(b.map));
	vec_push(bind_lines(), &b);
}

void	bind_lines_free(void)
{
	t_bind_line	*a;
	size_t		i;

	a = (t_bind_line *)bind_lines()->ctx;
	i = 0;
	while (i < bind_lines()->len)
	{
		xfree(a[i].map);
		xfree(a[i].line);
		i++;
	}
	xfree(bind_lines()->ctx);
	*bind_lines() = (t_vec){.elem_size = sizeof(t_bind_line)};
}
