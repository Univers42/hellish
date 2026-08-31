/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_zsh.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 13:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 04:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* The zsh prompt language, complete. In zsh, PS1 and PROMPT are the same
** parameter, and this reader answers for both: PROMPT always, and PS1 when
** the zsh dialect is armed (set -o zsh / emulate zsh) -- never from
** guessing at the text, because `%` is an ordinary character in a bash
** prompt and anyone with a literal percent in a legacy PS1 would have
** watched it disappear.
**
** It stays a FRONTEND: every escape is rewritten into the backslash
** language (or into text computed right here, for the escapes bash has no
** spelling for) and the one renderer does the work -- one expander, one
** width model, no drift.
**
** Every semantic below was MEASURED against the zsh 5.9 oracle
** (tests/build_zsh_oracle.sh), not read out of the manual: that is how
** "%# is % not $", "an unknown escape renders NOTHING", "a trailing lone
** % is dropped" and "%b is a full reset, not bold-off" were caught.
** tests/zsh_prompt_parity_test.py diffs `print -P` against that oracle
** byte for byte.
**
** The families live in their own files: numbers, colours, effects and the
** clock (prompt_zsh2.c), identity and psvar (prompt_zsh3.c), paths
** (prompt_zsh4.c), conditionals (prompt_zsh5.c), truncation
** (prompt_zsh6.c). This file is the dispatcher.
*/

/* The one-character escapes that map straight onto a backslash spelling.
   %h and %! are the history number; %N/%x answer "what file am I", which
   is how `${(%):-%N}` opens half the plugin corpus. */
static const char	*zsh_simple(char c)
{
	if (c == 'n')
		return ("\\u");
	if (c == 'm')
		return ("\\h");
	if (c == 'M')
		return ("\\H");
	if (c == '?')
		return ("$?");
	if (c == 'j')
		return ("\\j");
	if (c == 'h' || c == '!')
		return ("\\!");
	if (c == '%')
		return ("%");
	if (c == 'N' || c == 'x')
		return ("\\I");
	if (c == 'L')
		return ("${SHLVL}");
	if (c == 'i' || c == 'I')
		return ("${LINENO}");
	return (NULL);
}

/* zsh names eight colours; a number passes through; anything else is -1
   so the caller can try the #rrggbb form. */
int	zsh_color_code(const char *name, int len)
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
	if (len == 0 || !ft_isdigit(name[0]))
		return (-1);
	code = 0;
	i = 0;
	while (i < len && name[i] >= '0' && name[i] <= '9')
		code = code * 10 + (name[i++] - '0');
	return (code);
}

/* The optional numeric argument between `%` and the escape letter:
   %3~, %-1d, %2v. A bare '-' is not a number, so %-foo stays untouched. */
void	zsh_num(t_zesc *z)
{
	int	j;
	int	neg;

	j = z->j;
	neg = 0;
	if (z->f[j] == '-' && ft_isdigit(z->f[j + 1]))
	{
		neg = 1;
		j++;
	}
	if (!ft_isdigit(z->f[j]))
		return ;
	z->n = 0;
	while (ft_isdigit(z->f[j]))
		z->n = z->n * 10 + (z->f[j++] - '0');
	if (neg)
		z->n = -z->n;
	z->has_n = true;
	z->j = j;
}

/* One `%` escape. z->j sits on the character after `%` and any numeric
   argument; each family consumes what it recognises and answers true. An
   escape nobody claims renders NOTHING -- measured: zsh consumes unknown
   escapes, it does not print them. */
static void	zsh_escape(t_shell *state, t_string *out, t_zesc *z)
{
	const char	*rep;
	char		c;

	c = z->f[z->j];
	if (!c)
		return ;
	z->j++;
	if (zsh_color(out, z, c) || zsh_effects(out, c) || zsh_time(out, z, c))
		return ;
	if (zsh_cwd(state, out, z, c) || zsh_cond(state, out, z, c))
		return ;
	if (zsh_trunc(state, out, z, c) || zsh_ident(state, out, z, c))
		return ;
	rep = zsh_simple(c);
	if (rep)
		vec_push_str(out, (char *)rep);
}

/* Rewrite a whole prompt string into the backslash language. The caller
   renders the result with ps1_render, so nothing downstream knows which
   syntax was used. Re-entrant on purpose: conditionals and truncation
   hand their chosen sub-text back through here. */
t_string	zsh_to_ps1(t_shell *state, const char *fmt)
{
	t_string	out;
	t_zesc		z;
	int			i;

	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	while (fmt[i])
	{
		if (fmt[i] == '%' && fmt[i + 1])
		{
			z = (t_zesc){.f = fmt, .j = i + 1};
			zsh_num(&z);
			zsh_escape(state, &out, &z);
			i = z.j;
		}
		else if (fmt[i] == '%')
			i++;
		else
			vec_push_char(&out, fmt[i++]);
	}
	vec_push_char(&out, '\0');
	out.len--;
	return (out);
}
