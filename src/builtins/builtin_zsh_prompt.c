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

/* vcs_info's format language, the working subset: %b branch, %s the vcs
   name, %u the unstaged marker, %% a percent. Every other %x is consumed
   empty -- the zsh ones (%a action, %c staged, %r root ...) report state
   this reader does not track, and an empty field is what zsh itself
   renders for them at rest. */
static void	vcs_expand(t_string *out, const char *f, const char *b,
				int dirty)
{
	while (*f)
	{
		if (f[0] != '%' || !f[1])
		{
			vec_push_char(out, *f);
			f++;
			continue ;
		}
		if (f[1] == 'b')
			vec_push_str(out, (char *)b);
		else if (f[1] == 's')
			vec_push_str(out, "git");
		else if (f[1] == 'u' && dirty)
			vec_push_str(out, "*");
		else if (f[1] == '%')
			vec_push_char(out, '%');
		f += 2;
	}
}

/* `vcs_info`: fill vcs_info_msg_0_ from the prompt's own fork-free cached
   git reader -- the same source \g uses, so the two can never disagree
   about the branch. The format honours the captured zstyle (see
   builtin_zstyle); outside a repository the message is empty, exactly
   what a `${vcs_info_msg_0_}` prompt wants to interpolate. */
int	builtin_vcs_info(t_shell *state, t_vec argv)
{
	t_string	out;
	char		*branch;
	char		*fmt;
	int			dirty;

	(void)argv;
	vec_init(&out);
	out.elem_size = 1;
	get_git_info(&branch, &dirty);
	fmt = env_expand(state, "HELLISH_VCS_FORMAT");
	if (!fmt || !*fmt)
		fmt = " (%b)";
	if (branch)
		vcs_expand(&out, fmt, branch, dirty);
	vec_push_char(&out, '\0');
	env_set(&state->env, env_create(ft_strdup("vcs_info_msg_0_"),
			ft_strdup((char *)out.ctx), false));
	xfree(out.ctx);
	xfree(branch);
	return (0);
}

/* `zstyle`: the two keys vcs_info actually reads are captured and honoured
   silently; everything else still goes to the loud once-per-session stub,
   because pretending to store a completion style hellish cannot consult
   would be worse than saying so. */
int	builtin_zstyle(t_shell *state, t_vec argv)
{
	char	**av;

	av = (char **)argv.ctx;
	if (argv.len >= 4 && ft_strnstr(av[1], ":vcs_info", ft_strlen(av[1])))
	{
		if (!ft_strcmp(av[2], "formats"))
			return (env_set(&state->env, env_create(
						ft_strdup("HELLISH_VCS_FORMAT"),
						ft_strdup(av[3]), false)), 0);
		if (!ft_strcmp(av[2], "actionformats"))
			return (env_set(&state->env, env_create(
						ft_strdup("HELLISH_VCS_AFORMAT"),
						ft_strdup(av[3]), false)), 0);
	}
	return (builtin_zunsupported(state, argv));
}
