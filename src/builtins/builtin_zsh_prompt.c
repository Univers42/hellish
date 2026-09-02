/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_prompt.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 02:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/01 02:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_builtins.h"
#include "env.h"
#include "prompt.h"

/* The zsh prompt-customization vocabulary -- issue #91. Every zsh prompt
** tutorial on the internet opens with
**
**     autoload -Uz colors && colors
**     autoload -Uz vcs_info
**     zstyle ':vcs_info:git:*' formats ' %b'
**     precmd() { vcs_info }
**     PROMPT='%~ ${vcs_info_msg_0_} %# '
**
** and every line of it used to fail here: `colors` and `vcs_info` live in
** zsh's fpath, which hellish does not ship, so autoload silently skipped
** them and the calls died as "command not found". They are builtins now,
** which also makes autoload's skip exactly right: the name resolves.
*/

/* One colour family -- fg, fg_bold, bg, ... -- as the associative array
   zsh's colors function defines: eight names plus `default`, each mapping
   to its SGR sequence. base is 30 (foreground) or 40 (background). */
static void	colors_family(t_shell *state, const char *name, int base,
				bool bold)
{
	static const char *const	n[9] = {"black", "red", "green", "yellow",
		"blue", "magenta", "cyan", "white", "default"};
	char						buf[24];
	char						*val;
	char						*nv;
	int							i;

	val = ft_strdup("\x1c");
	i = 0;
	while (val && i < 9)
	{
		if (bold)
			snprintf(buf, sizeof(buf), "\033[1;%dm", base + i + (i == 8));
		else
			snprintf(buf, sizeof(buf), "\033[%dm", base + i + (i == 8));
		nv = assoc_with_set(val, n[i], ft_strlen(n[i]), buf);
		xfree(val);
		val = nv;
		i++;
	}
	if (val)
		env_set(&state->env, env_create(ft_strdup(name), val, false));
}

/* `colors`: the variables zsh's own colors function defines, so a theme
   can write $fg[cyan], $fg_bold[red], $reset_color and friends. */
int	builtin_colors(t_shell *state, t_vec argv)
{
	(void)argv;
	colors_family(state, "fg", 30, false);
	colors_family(state, "fg_bold", 30, true);
	colors_family(state, "fg_no_bold", 30, false);
	colors_family(state, "bg", 40, false);
	colors_family(state, "bg_bold", 40, true);
	colors_family(state, "bg_no_bold", 40, false);
	env_set(&state->env, env_create(ft_strdup("reset_color"),
			ft_strdup("\033[0m"), false));
	env_set(&state->env, env_create(ft_strdup("bold_color"),
			ft_strdup("\033[1m"), false));
	return (0);
}
