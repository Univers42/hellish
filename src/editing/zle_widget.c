/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle_widget.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 15:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "zle.h"

/* ZLE -- zsh's programmable line editor, on top of readline.
**
** A plugin registers a shell function as a WIDGET, binds it to a key, and
** the function edits the line by assigning to shell variables:
**
**     sudo-command-line() {
**       [[ -z $BUFFER ]] && LBUFFER="$(fc -ln -1)"
**       if [[ $BUFFER == sudo\ * ]]; then
**         LBUFFER="${LBUFFER#sudo }"
**       else
**         LBUFFER="sudo $LBUFFER"
**       fi
**       zle redisplay
**     }
**     zle -N sudo-command-line
**     bindkey '\e\e' sudo-command-line
**
** That is oh-my-zsh's sudo plugin, one of the most-installed there is, and
** it is the whole shape: read BUFFER, write LBUFFER, ask for a redraw.
**
** WHAT READLINE DOES NOT HAVE, and what this supplies:
**
**   a shell FUNCTION as a keybinding   readline binds a C function pointer.
**                                      zle_dispatch is that pointer, once,
**                                      and it looks the widget up by the key
**                                      sequence that invoked it.
**   BUFFER as a writable variable      readline owns rl_line_buffer and
**                                      rl_point in C. zle_publish copies
**                                      them into the environment before the
**                                      widget runs and zle_collect writes
**                                      them back after.
**
** SCOPE, stated plainly. `region_highlight` -- an array of (start, end,
** style) the editor repaints from -- has no readline equivalent at all, and
** it is what zsh-syntax-highlighting is built on. Not implemented, and
** `zle` reports so rather than accepting the call.
*/

/* The registry, and the key bindings that point into it. Both are
   function-local statics for the same reason the glob option cells are:
   readline's callback takes no context, so anything it needs has to be
   reachable without one. */
t_vec	*zle_widgets(void)
{
	static t_vec	v;

	if (!v.elem_size)
	{
		vec_init(&v);
		v.elem_size = sizeof(t_zle_widget);
	}
	return (&v);
}

/* Register `name` as a widget backed by shell function `fn` (or by itself
   when fn is NULL, which is `zle -N name` with no explicit function --
   zsh's default is a function of the same name). Re-registering replaces. */
void	zle_widget_add(const char *name, const char *fn)
{
	t_zle_widget	w;
	t_zle_widget	*old;

	if (!fn)
		fn = name;
	old = zle_widget_get(name);
	if (old)
	{
		xfree(old->fn);
		old->fn = ft_strdup(fn);
		return ;
	}
	w.name = ft_strdup(name);
	w.fn = ft_strdup(fn);
	vec_push(zle_widgets(), &w);
}

t_zle_widget	*zle_widget_get(const char *name)
{
	t_zle_widget	*a;
	size_t			i;

	a = (t_zle_widget *)zle_widgets()->ctx;
	i = 0;
	while (i < zle_widgets()->len)
	{
		if (!ft_strcmp(a[i].name, name))
			return (&a[i]);
		i++;
	}
	return (NULL);
}

/* Release the registry. Called from free_all_state like every other table
   the session owns. */
void	zle_widgets_free(void)
{
	t_zle_widget	*a;
	size_t			i;

	a = (t_zle_widget *)zle_widgets()->ctx;
	i = 0;
	while (i < zle_widgets()->len)
	{
		xfree(a[i].name);
		xfree(a[i].fn);
		i++;
	}
	xfree(zle_widgets()->ctx);
	*zle_widgets() = (t_vec){.elem_size = sizeof(t_zle_widget)};
}
