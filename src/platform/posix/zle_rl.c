/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle_rl.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 15:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 15:20:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "zle.h"
#include "env.h"
#include <readline/readline.h>

/* The bridge: a shell function invoked as a readline keybinding.
**
** readline binds a C FUNCTION POINTER. zle_dispatch is that pointer -- one
** of it, shared by every widget -- and it finds out which widget it is by
** asking rl_executing_keyseq what was typed. That is why the bindings are
** kept in a table of our own (zle_bind.c) rather than only in readline's
** keymap: readline can say "this key fired" but not "and it means the shell
** function named X".
**
** exec_string does NOT take the string it is handed: it alias-expands into
** a copy and frees only that.  The widget's name is passed straight from
** the registry -- a strdup here would leak one name per keypress, on a path
** no `-c` test can reach because it needs a live readline.
**
** THE SHELL STATE IS THE SHELL'S. The line is read in the shell process,
** so what a widget does stays done -- a `cd`, a variable, a function --
** as with zsh widgets and bash's `bind -x`. rl_shell_exec runs it with
** readline's signals and terminal settings handled around the call, and
** its BUFFER/LBUFFER/RBUFFER/CURSOR are scoped to it (zle_params.c).
** Under HELLISH_RL_FORK=1 it runs in the reader's child as it used to, and
** only BUFFER edits and a `cd` (zle_cwd.c) come back.
*/

/* The state widgets run against, parked where a readline callback --
   which takes no context -- can reach it. Non-NULL exactly while a line
   is being edited. */
t_shell	**zle_state_cell(void)
{
	static t_shell	*st;

	return (&st);
}

/* Copy readline's line and cursor into the shell variables a widget reads.
   LBUFFER and RBUFFER are the halves either side of the cursor, which is
   how zsh code inserts at the cursor without touching the rest.
     Not static: a built-in widget that edits readline's line directly
   (`zle kill-buffer`) has to refresh these too, or zle_collect writes the
   stale text straight back over the edit. See builtin_zle.c. */
void	zle_publish(t_shell *state)
{
	char	*s;

	s = rl_line_buffer;
	if (!s)
		s = "";
	env_set(&state->env, env_create(ft_strdup("BUFFER"),
			ft_strdup(s), false));
	env_set(&state->env, env_create(ft_strdup("LBUFFER"),
			ft_strndup(s, (size_t)rl_point), false));
	env_set(&state->env, env_create(ft_strdup("RBUFFER"),
			ft_strdup(s + rl_point), false));
	env_set(&state->env, env_create(ft_strdup("CURSOR"),
			ft_itoa(rl_point), false));
}

/* Read the variables back and install them in readline.
**
** LBUFFER/RBUFFER win over BUFFER when either changed, because that is what
** a widget writes: zsh's sudo assigns LBUFFER and never touches BUFFER, and
** taking BUFFER's stale value would undo the edit. When only BUFFER changed,
** it is used and the cursor goes to the end -- zsh's own rule.
*/
static void	zle_collect(t_shell *state, const char *was)
{
	char	*l;
	char	*r;
	char	*joined;

	l = env_expand(state, "LBUFFER");
	if (!l)
		l = "";
	r = env_expand(state, "RBUFFER");
	if (!r)
		r = "";
	joined = ft_strjoin(l, r);
	if (joined && !ft_strcmp(joined, was))
		joined = (xfree(joined), ft_strdup(env_expand(state, "BUFFER")));
	if (!joined)
		return ;
	rl_replace_line(joined, 0);
	rl_point = (int)ft_strlen(l);
	if (rl_point > rl_end)
		rl_point = rl_end;
	xfree(joined);
}

/* One widget run: the line goes in through BUFFER and friends, comes
   back out of them, and those names are then what they were before. */
static void	zle_run(t_shell *state, t_zle_widget *w)
{
	t_zle_saved	saved;
	char		*was;

	was = ft_strdup("");
	if (rl_line_buffer)
		was = (xfree(was), ft_strdup(rl_line_buffer));
	zle_params_save(state, &saved);
	zle_publish(state);
	rl_shell_exec(state, w->fn);
	if (was)
		zle_collect(state, was);
	zle_params_restore(state, &saved);
	xfree(was);
}

/* Run the widget bound to the key sequence that just fired. */
int	zle_dispatch(int count, int key)
{
	const char		*name;
	t_zle_widget	*w;
	t_shell			*state;

	(void)count;
	(void)key;
	state = *zle_state_cell();
	name = zle_bind_widget(rl_executing_keyseq);
	if (!state || !name)
		return (0);
	w = zle_widget_get(name);
	if (!w)
		return (0);
	if (w->bashx)
		zle_run_x(state, w);
	else
		zle_run(state, w);
	if (zle_active())
		rl_redisplay();
	return (0);
}
