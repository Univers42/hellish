/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle_rl3.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 15:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 15:40:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "zle.h"
#include <readline/readline.h>

int	exec_string(t_shell *state, char *content);

/* Invoke one registered widget from inside another.

   No strdup on the way in: exec_string alias-expands into a COPY and frees
   only that, so the string it is handed still belongs to the caller. The
   strdup that used to be here leaked one widget name per invocation, on a
   path no `-c` case can reach because it needs a live readline -- which is
   why neither ASan nor the allocator oracle ever saw it. zle_dispatch
   passes w->fn straight through, for the same reason. */
int	zle_run_widget(t_shell *state, const char *name)
{
	t_zle_widget	*w;

	w = zle_widget_get(name);
	if (!w || !w->fn)
		return (1);
	return (exec_string(state, w->fn));
}

/* Install every recorded binding into readline's current keymap, and mark
** this process as the one inside the editor.
**
** Called from the readline child after the editing mode is set up, because
** setup_emacs_mode/setup_vi_mode replace the keymap: a binding installed
** before them would be discarded, silently, and the key would simply do
** nothing.
**
** rl_initialize() first, for the same class of reason: readline builds its
** keymaps there, and rl_bind_keyseq before it binds into a map that is
** about to be replaced. The failure is identical -- the key does nothing,
** with no error anywhere -- which is why both orderings are spelled out
** rather than left to be rediscovered.
*/
void	zle_install(t_shell *state)
{
	t_zle_bind	*a;
	size_t		i;

	*zle_state_cell() = state;
	rl_initialize();
	a = (t_zle_bind *)zle_binds()->ctx;
	i = 0;
	while (i < zle_binds()->len)
	{
		zle_bind_raw(&a[i]);
		rl_bind_keyseq(a[i].seq, zle_dispatch);
		i++;
	}
}

/* Record what readline will REPORT for this binding, which is not what the
** plugin wrote.
**
** `bindkey '\e\e' sudo-command-line` names the sequence in zsh's escape
** syntax; readline understands that syntax when BINDING, but at dispatch
** time rl_executing_keyseq hands back the RAW BYTES that were typed -- two
** 0x1b, not the four characters backslash-e-backslash-e. Looking the widget
** up by the written form therefore never matched, and the key did nothing at
** all: no error, no message, just an unchanged line.
**
** rl_translate_keyseq is readline's own converter, so the two spellings
** cannot drift apart the way a hand-rolled unescape would. */
void	zle_bind_raw(t_zle_bind *b)
{
	char	buf[128];
	int		n;

	if (b->raw)
		return ;
	n = 0;
	if (rl_translate_keyseq(b->seq, buf, &n) || n <= 0)
		b->raw = ft_strdup(b->seq);
	else
		b->raw = ft_strndup(buf, (size_t)n);
}
