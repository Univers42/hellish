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

int	exec_string(t_shell *state, char *content);

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
** THE SHELL STATE IS THE FORKED CHILD'S. readline runs in a child of the
** shell (see bg_readline), so the widget's edits to BUFFER survive -- the
** line is what the child sends back -- while anything else it changes does
** NOT. A widget that runs `cd` changes the child's directory and the parent
** never learns. That is a real boundary, not an oversight: it is why
** oh-my-zsh's sudo works here and dirhistory, whose widgets cd, does not.
*/

/* The state the child's widgets run against, parked where a readline
   callback -- which takes no context -- can reach it. */
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

/* Run the widget bound to the key sequence that just fired. */
int	zle_dispatch(int count, int key)
{
	const char		*name;
	t_zle_widget	*w;
	t_shell			*state;
	char			*was;

	(void)count;
	(void)key;
	state = *zle_state_cell();
	name = zle_bind_widget(rl_executing_keyseq);
	if (!state || !name)
		return (0);
	w = zle_widget_get(name);
	if (!w)
		return (0);
	was = ft_strdup("");
	if (rl_line_buffer)
		was = (xfree(was), ft_strdup(rl_line_buffer));
	zle_publish(state);
	exec_string(state, w->fn);
	if (was)
		zle_collect(state, was);
	xfree(was);
	rl_redisplay();
	return (0);
}
