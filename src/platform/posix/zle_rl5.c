/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle_rl5.c                                          :+:      :+:    :+:   */
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
#include "env.h"
#include "ft_builtins.h"
#include <readline/readline.h>

/* `bind -x`: a key that runs a shell command, the way bash runs it
** (bash_execute_unix_command). The command sees the line as READLINE_LINE,
** the cursor as READLINE_POINT and the mark as READLINE_MARK -- counted in
** CHARACTERS, not bytes -- and whatever it leaves in them becomes the line,
** the cursor and the mark. The visible line is cleared first, so the
** command's output starts on a clean row, and the prompt and line are drawn
** again after it. The three names exist only while the command runs.
** fzf's key bindings for bash are built on exactly this. */

/* Characters before byte offset `bytes`, and the byte offset of character
   `chars` (the end of the line past the last one). UTF-8: a character is
   every byte that is not a continuation byte. */
static int	zle_x_chars(const char *s, int bytes)
{
	int	i;
	int	n;

	i = 0;
	n = 0;
	while (i < bytes && s[i])
		if (((unsigned char)s[i++] & 0xC0) != 0x80)
			n++;
	return (n);
}

static int	zle_x_bytes(const char *s, int chars)
{
	int	i;
	int	n;

	i = 0;
	n = 0;
	while (s[i])
	{
		if (((unsigned char)s[i] & 0xC0) != 0x80 && n++ == chars)
			return (i);
		i++;
	}
	return (i);
}

static void	zle_x_publish(t_shell *state)
{
	const char	*s;

	s = rl_line_buffer;
	if (!s)
		s = "";
	env_set(&state->env, env_create(ft_strdup("READLINE_LINE"),
			ft_strdup(s), true));
	env_set(&state->env, env_create(ft_strdup("READLINE_POINT"),
			ft_itoa(zle_x_chars(s, rl_point)), true));
	env_set(&state->env, env_create(ft_strdup("READLINE_MARK"),
			ft_itoa(zle_x_chars(s, rl_mark)), true));
}

/* The number in `name`, as a byte offset into the new line, or -1 when
   it is unset or not a number -- bash leaves the position alone then. */
static int	zle_x_offset(t_shell *state, const char *name)
{
	char	*v;
	int		i;

	v = env_expand(state, (char *)name);
	if (!v || !*v)
		return (-1);
	i = (v[0] == '-');
	while (v[i] && ft_isdigit(v[i]))
		i++;
	if (v[i] || ft_atoi(v) < 0)
		return (-1);
	return (zle_x_bytes(rl_line_buffer, ft_atoi(v)));
}

void	zle_run_x(t_shell *state, t_zle_widget *w)
{
	char	*line;
	int		at;

	rl_clear_visible_line();
	fflush(rl_outstream);
	zle_x_publish(state);
	rl_shell_exec(state, w->fn);
	line = env_expand(state, "READLINE_LINE");
	if (line && ft_strcmp(line, rl_line_buffer))
	{
		rl_replace_line(line, 0);
		rl_point = rl_end;
	}
	at = zle_x_offset(state, "READLINE_POINT");
	if (at >= 0)
		rl_point = at;
	at = zle_x_offset(state, "READLINE_MARK");
	if (at >= 0)
		rl_mark = at;
	try_unset(state, "READLINE_LINE");
	try_unset(state, "READLINE_POINT");
	try_unset(state, "READLINE_MARK");
	rl_forced_update_display();
}
