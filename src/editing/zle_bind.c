/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle_bind.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 15:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 15:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "zle.h"

/* The key bindings a plugin asked for, kept until readline exists to
** receive them.
**
** `bindkey` runs when the plugin is SOURCED -- at startup, in the parent
** shell. readline runs later, in a forked child, and its keymaps are set up
** fresh there (setup_emacs_mode / setup_vi_mode). So a binding installed at
** source time would be installed into a keymap that is about to be replaced,
** in a process that will never read a key.
**
** Recording them and replaying into each child is the only ordering that
** works, and it also gets the semantics right for free: a `bindkey` issued
** later (from a function, from another plugin) is picked up by the next
** prompt without any re-registration.
*/

t_vec	*zle_binds(void)
{
	static t_vec	v;

	if (!v.elem_size)
	{
		vec_init(&v);
		v.elem_size = sizeof(t_zle_bind);
	}
	return (&v);
}

/* Record one binding. A repeat of the same sequence REPLACES: a plugin that
   binds the same key in three keymaps (emacs, vicmd, viins -- which is what
   oh-my-zsh's sudo does) must not leave three entries fighting over it, and
   readline's keymaps are not per-editing-mode here. */
void	zle_bind_add(const char *seq, const char *widget)
{
	t_zle_bind	b;
	t_zle_bind	*a;
	size_t		i;

	a = (t_zle_bind *)zle_binds()->ctx;
	i = 0;
	while (i < zle_binds()->len)
	{
		if (!ft_strcmp(a[i].seq, seq))
		{
			xfree(a[i].widget);
			a[i].widget = ft_strdup(widget);
			return ;
		}
		i++;
	}
	b.seq = ft_strdup(seq);
	b.raw = NULL;
	b.widget = ft_strdup(widget);
	vec_push(zle_binds(), &b);
}

/* The widget bound to the RAW byte sequence readline just reported.
   Matched against `raw`, not `seq`: the plugin wrote `\e\e` and readline
   reports two ESC bytes. */
const char	*zle_bind_widget(const char *seq)
{
	t_zle_bind	*a;
	size_t		i;

	a = (t_zle_bind *)zle_binds()->ctx;
	i = 0;
	while (i < zle_binds()->len)
	{
		if (a[i].raw && !ft_strcmp(a[i].raw, seq))
			return (a[i].widget);
		i++;
	}
	return (NULL);
}

void	zle_binds_free(void)
{
	t_zle_bind	*a;
	size_t		i;

	a = (t_zle_bind *)zle_binds()->ctx;
	i = 0;
	while (i < zle_binds()->len)
	{
		xfree(a[i].seq);
		xfree(a[i].raw);
		xfree(a[i].widget);
		i++;
	}
	xfree(zle_binds()->ctx);
	*zle_binds() = (t_vec){.elem_size = sizeof(t_zle_bind)};
}
