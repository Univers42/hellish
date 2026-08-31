/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_zsh2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 04:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 04:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Text computed at rewrite time still has to survive ps1_render, where `$`
   starts an expansion and `\` an escape. `\044` is the octal spelling of
   `$` -- the only way to say "a literal dollar" in that language, because
   \$ already means "$ or # by uid". */
void	zsh_inject(t_string *out, const char *s)
{
	while (*s)
	{
		if (*s == '$')
			vec_push_str(out, "\\044");
		else if (*s == '\\')
			vec_push_str(out, "\\\\");
		else
			vec_push_char(out, *s);
		s++;
	}
}

/* %F{...}/%K{...}: the exact bytes zsh emits on a 256-colour terminal,
   measured -- a NAMED colour is the classic 3x/4x code, a number under 8
   too, 8..255 goes 38;5, and #rrggbb goes 38;2 truecolour. Wrapped in
   \[ \] so the width model counts zero columns. */
bool	zsh_color(t_string *out, t_zesc *z, char c)
{
	char	buf[48];
	int		code;
	int		j;
	int		base;

	if (c != 'F' && c != 'K')
		return (false);
	base = 38 + (c == 'K') * 10;
	if (z->f[z->j] != '{')
		return (snprintf(buf, sizeof(buf), "\\[\\e[%dm\\]", base + 1),
			vec_push_str(out, buf), true);
	j = z->j + 1;
	while (z->f[j] && z->f[j] != '}')
		j++;
	code = zsh_color_code(z->f + z->j + 1, j - z->j - 1);
	if (code >= 0 && code < 8)
		snprintf(buf, sizeof(buf), "\\[\\e[%dm\\]", base - 8 + code);
	else if (code >= 0)
		snprintf(buf, sizeof(buf), "\\[\\e[%d;5;%dm\\]", base, code);
	else if (!zsh_color_hex(buf, sizeof(buf), z->f + z->j + 1, base))
		buf[0] = '\0';
	vec_push_str(out, buf);
	return (z->j = j + (z->f[j] == '}'), true);
}

/* The visual toggles, byte-identical to zsh 5.9 on xterm-256color. %b is
   a FULL reset (\e[0m), not bold-off -- measured, and surprising. %{ and
   %} are zsh's spelling of the zero-width region, i.e. \[ and \]. %G, %_
   and %^ render nothing at a primary prompt; a stray %) is a literal
   parenthesis. */
bool	zsh_effects(t_string *out, char c)
{
	if (c == 'B')
		return (vec_push_str(out, "\\[\\e[1m\\]"), true);
	if (c == 'b')
		return (vec_push_str(out, "\\[\\e[0m\\]"), true);
	if (c == 'U')
		return (vec_push_str(out, "\\[\\e[4m\\]"), true);
	if (c == 'u')
		return (vec_push_str(out, "\\[\\e[24m\\]"), true);
	if (c == 'S')
		return (vec_push_str(out, "\\[\\e[7m\\]"), true);
	if (c == 's')
		return (vec_push_str(out, "\\[\\e[27m\\]"), true);
	if (c == 'f')
		return (vec_push_str(out, "\\[\\e[39m\\]"), true);
	if (c == 'k')
		return (vec_push_str(out, "\\[\\e[49m\\]"), true);
	if (c == 'E')
		return (vec_push_str(out, "\\[\\e[K\\]"), true);
	if (c == '{')
		return (vec_push_str(out, "\\["), true);
	if (c == '}')
		return (vec_push_str(out, "\\]"), true);
	if (c == ')')
		return (vec_push_char(out, ')'), true);
	return (c == 'G' || c == '_' || c == '^');
}

/* %D{...}: the user's own strftime format, copied through to \D{...}.
   Empty braces render NOTHING -- measured; bash's \D{} would answer with
   the locale clock, and zsh disagrees. */
static void	zsh_time_fmt(t_string *out, t_zesc *z)
{
	int	j;

	j = z->j + 1;
	while (z->f[j] && z->f[j] != '}')
		j++;
	if (j == z->j + 1)
		return ((void)(z->j = j + (z->f[j] == '}')));
	vec_push_str(out, "\\D{");
	z->j++;
	while (z->j < j)
		vec_push_char(out, z->f[z->j++]);
	vec_push_char(out, '}');
	z->j = j + (z->f[j] == '}');
}

/* The clock family, rewritten onto the PS1 engine's \D{strftime}. The
   formats reproduce zsh's unpadded spellings: %T is "3:54", not "03:54",
   and %t is " 3:54AM" with strftime's %l space-pad. %D{...} passes the
   user's own format straight through. */
bool	zsh_time(t_string *out, t_zesc *z, char c)
{
	if (c == 'T')
		return (vec_push_str(out, "\\D{%-H:%M}"), true);
	if (c == 't' || c == '@')
		return (vec_push_str(out, "\\D{%l:%M%p}"), true);
	if (c == '*')
		return (vec_push_str(out, "\\D{%-H:%M:%S}"), true);
	if (c == 'W')
		return (vec_push_str(out, "\\D{%m/%d/%y}"), true);
	if (c == 'w')
		return (vec_push_str(out, "\\D{%a %-d}"), true);
	if (c != 'D')
		return (false);
	if (z->f[z->j] != '{')
		return (vec_push_str(out, "\\D{%y-%m-%d}"), true);
	return (zsh_time_fmt(out, z), true);
}
