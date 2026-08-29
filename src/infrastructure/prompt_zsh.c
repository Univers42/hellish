/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_zsh.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 13:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 13:40:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "env.h"

/* The zsh-style `%` prompt language, asked for in issue #69:
**
**     PROMPT='%F{green}%n@%m%f %~ %? '
**
** It is a FRONTEND, not a second prompt engine. Every `%` escape is rewritten
** into the backslash form and the existing renderer does the work, so both
** syntaxes share one expander, one width model and one set of extensions --
** which is the only way they can be kept from drifting apart.
**
** Opt-in by variable, not by sniffing: PROMPT selects this reader, PS1 the
** bash one. Sniffing would have to guess what a bare `%` means, and `%` is an
** ordinary character in a bash prompt -- anyone with a literal percent in
** their PS1 would have watched it disappear.
**
** Covered: %n user, %m short host, %M full host, %~ cwd (~), %d /%/ full cwd,
** %# $ or #, %? exit status, %j jobs, %B/%b bold, %F{c}/%f fg, %K{c}/%k bg,
** %% a literal percent. hellish's own badges stay available by their
** backslash names, because they have no zsh counterpart to borrow. */

static const char	*zsh_simple(char c)
{
	if (c == 'n')
		return ("\\u");
	if (c == 'm')
		return ("\\h");
	if (c == 'M')
		return ("\\H");
	if (c == '~')
		return ("\\w");
	if (c == 'd' || c == '/')
		return ("\\W");
	if (c == '#')
		return ("\\$");
	if (c == '?')
		return ("$?");
	if (c == 'j')
		return ("\\j");
	if (c == '%')
		return ("%");
	return (NULL);
}

/* zsh names eight colours; anything else is passed through as a number, so
   %F{81} works the way it does in zsh. */
static int	zsh_color_code(const char *name, int len)
{
	static const char	*n[8] = {"black", "red", "green", "yellow",
		"blue", "magenta", "cyan", "white"};
	int					i;
	int					code;

	i = 0;
	while (i < 8)
	{
		if ((int)ft_strlen(n[i]) == len && ft_strncmp(n[i], name, len) == 0)
			return (i);
		i++;
	}
	code = 0;
	i = 0;
	while (i < len && name[i] >= '0' && name[i] <= '9')
		code = code * 10 + (name[i++] - '0');
	return (code);
}

/* %F{x} / %K{x}: emit an SGR sequence inside \[ \] so the width model still
   counts zero columns for it. */
static void	zsh_color(t_string *out, const char *f, int *i, bool fg)
{
	char	buf[32];
	int		j;
	int		code;

	j = *i + 2;
	if (f[j] != '{')
		return ((void)(*i += 2));
	j++;
	*i = j;
	while (f[j] && f[j] != '}')
		j++;
	code = zsh_color_code(f + *i, j - *i);
	snprintf(buf, sizeof(buf), "\\[\\e[%d;5;%dm\\]", 38 + (!fg) * 10, code);
	vec_push_str(out, buf);
	*i = j + (f[j] == '}');
}

/* One `%` escape into its backslash equivalent. */
static void	zsh_escape(t_string *out, const char *f, int *i)
{
	const char	*rep;
	char		c;

	c = f[*i + 1];
	if (c == 'F' || c == 'K')
		return (zsh_color(out, f, i, c == 'F'));
	*i += 2;
	if (c == 'f' || c == 'k' || c == 'b')
		return ((void)vec_push_str(out, "\\[\\e[0m\\]"));
	if (c == 'B')
		return ((void)vec_push_str(out, "\\[\\e[1m\\]"));
	rep = zsh_simple(c);
	if (rep)
		return ((void)vec_push_str(out, (char *)rep));
	vec_push_char(out, '%');
	vec_push_char(out, c);
}

/* Rewrite a whole PROMPT into the backslash language. The caller renders the
   result with ps1_render, so nothing downstream knows which syntax was used. */
t_string	zsh_to_ps1(const char *fmt)
{
	t_string	out;
	int			i;

	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	while (fmt[i])
	{
		if (fmt[i] == '%' && fmt[i + 1])
			zsh_escape(&out, fmt, &i);
		else
			vec_push_char(&out, fmt[i++]);
	}
	vec_push_char(&out, '\0');
	out.len--;
	return (out);
}
