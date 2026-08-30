/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zle.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 15:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 15:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "zle.h"

/* `zle` and `bindkey`.
**
**     zle -N name [fn]     register a widget
**     zle <widget>         invoke one (only from inside another widget)
**     zle                  bare: "is the line editor active?"
**     bindkey [-M map] seq widget
**
** The bare `zle` is the one worth explaining. Plugins guard on it:
**
**     zle && zle redisplay   # only run redisplay if zle is enabled
**
** so it must answer FALSE outside the editor and TRUE inside a widget --
** otherwise every plugin that guards this way would try to redraw a line
** that is not being edited. zle_state_cell() is non-NULL only inside the
** readline child, which is exactly that distinction.
*/

/* The built-in widgets a plugin invokes by name. Only the ones the corpus
   uses; anything else is reported rather than quietly accepted, because a
   widget that appears to run and does nothing is indistinguishable from a
   working one until the user presses the key. */
static int	zle_builtin(t_shell *state, const char *name)
{
	if (!ft_strcmp(name, "redisplay") || !ft_strcmp(name, "reset-prompt")
		|| !ft_strcmp(name, ".redisplay"))
		return (zle_do_redisplay(), 0);
	if (!ft_strcmp(name, "kill-buffer") || !ft_strcmp(name, ".kill-buffer"))
		return (zle_do_kill_buffer(), zle_publish(state), 0);
	if (!ft_strcmp(name, "accept-line") || !ft_strcmp(name, ".accept-line"))
		return (zle_do_accept_line(), 0);
	if (zle_widget_get(name))
		return (zle_run_widget(state, name));
	return (ft_eprintf("%s: zle: %s: no such widget\n", state->ctx, name), 1);
}

/* zle -N name [fn] */
static int	zle_register(t_shell *state, t_vec argv, size_t i)
{
	char	**av;

	av = (char **)argv.ctx;
	if (i >= argv.len)
		return (ft_eprintf("%s: zle: -N: widget name expected\n",
				state->ctx), 1);
	if (i + 1 < argv.len)
		zle_widget_add(av[i], av[i + 1]);
	else
		zle_widget_add(av[i], NULL);
	return (0);
}

int	builtin_zle(t_shell *state, t_vec argv)
{
	char	**av;

	av = (char **)argv.ctx;
	if (argv.len < 2)
		return (zle_active() == false);
	if (!ft_strcmp(av[1], "-N"))
		return (zle_register(state, argv, 2));
	if (av[1][0] == '-')
		return (0);
	if (!zle_active())
		return (ft_eprintf("%s: zle: can only be called from a widget\n",
				state->ctx), 1);
	return (zle_builtin(state, av[1]));
}

/* bindkey [-M keymap] sequence widget.
     -M is accepted and its argument discarded: readline has one keymap per
   editing mode and switches between them itself, so binding the same key in
   emacs, vicmd and viins -- which is what oh-my-zsh's sudo does, three
   calls -- is one binding here. Recording three would have them overwrite
   each other and the last would win, which is the same answer by accident
   rather than by design. */
int	builtin_bindkey(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;

	av = (char **)argv.ctx;
	i = 1;
	while (i < argv.len && av[i][0] == '-' && av[i][1])
	{
		if (av[i][1] == 'M' && i + 1 < argv.len)
			i++;
		i++;
	}
	if (i + 1 >= argv.len)
		return (0);
	zle_bind_add(av[i], av[i + 1]);
	(void)state;
	return (0);
}
