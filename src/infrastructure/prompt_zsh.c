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
** The families live in their own files: colours, effects and the clock
** (prompt_zsh2.c), identity and psvar (prompt_zsh3.c), paths
** (prompt_zsh4.c), conditionals (prompt_zsh5.c), truncation
** (prompt_zsh6.c), and the bilingual-PS1 rules -- literal unknowns,
** verbatim $-spans -- with the simple map (prompt_zsh7.c). This file is
** the dispatcher.
*/

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
   escape nobody claims renders NOTHING under strict zsh -- measured: zsh
   consumes unknown escapes -- and stays LITERAL in the bilingual PS1
   reader, which is what keeps a legacy `100% ` intact. */
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
		return ((void)vec_push_str(out, (char *)rep));
	if (!z->strict)
		zsh_lit(out, z);
}

/* Rewrite a whole prompt string into the backslash language. The caller
   renders the result with ps1_render, so nothing downstream knows which
   syntax was used. Re-entrant on purpose: conditionals and truncation
   hand their chosen sub-text back through here, strictness included.
   The mixed reader copies $-expansion and \D{...} spans through
   verbatim first, so their strftime percents never reach the escapes;
   the strict reader substitutes simple parameters first and converts
   their values too, which is zsh's PROMPT_SUBST order (prompt_zsh8.c). */
t_string	zsh_to_ps1(t_shell *state, const char *fmt, bool strict)
{
	t_string	out;
	t_zesc		z;
	int			i;

	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	while (fmt[i])
	{
		if ((!strict && zsh_span_copy(&out, fmt, &i))
			|| (strict && zsh_param_subst(state, &out, fmt, &i)))
			continue ;
		if (fmt[i] == '%' && fmt[i + 1])
		{
			z = (t_zesc){.f = fmt, .j = i + 1, .strict = strict, .start = i};
			zsh_num(&z);
			zsh_escape(state, &out, &z);
			i = z.j;
		}
		else if (fmt[i] == '%' && strict)
			i++;
		else
			vec_push_char(&out, fmt[i++]);
	}
	return (vec_push_char(&out, '\0'), out.len--, out);
}
