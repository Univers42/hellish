/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_ps1d.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "env.h"

char	*expand_param_format(t_shell *state, const char *s, int slen, bool dq);

/* The two PS1 forms a stock distro prompt is built out of, and that a
   hand-rolled prompt reader gets wrong. Both were found rendering Ubuntu's
   default PS1 through a login hellish (issue #41), where

     ${debian_chroot:+($debian_chroot)}\[\033[01;32m\]\u@\h...

   came out on screen as the literal text ":+()}\033[01;32mdlesieur@..."
   -- the operator half of the ${...} spat out raw, and every colour escape
   printed instead of applied. */

/* Offset of the '}' closing the ${...} whose '{' sits at line[0], honouring
   nested braces so ${a:-${b}} is consumed whole. 0 means unterminated. Same
   scan the heredoc expander uses; kept here rather than shared because that
   one is static to its own translation unit. */
static int	ps1_brace_span(const char *line)
{
	int	i;
	int	depth;

	i = 1;
	depth = 1;
	while (line[i] && depth > 0)
	{
		depth += (line[i] == '{');
		depth -= (line[i] == '}');
		if (depth == 0)
			return (i);
		i++;
	}
	return (0);
}

/* \nnn: the byte with that octal value, bash's PS1 escape for emitting a
   control character. Without it, the '\033' that opens every colour span in
   a distro PS1 fell through to the unknown-escape path and was printed as
   the four characters \ 0 3 3 -- the exact garbage in issue #41. Up to three
   octal digits, like bash. A \0 would truncate the prompt at the first NUL,
   so it contributes nothing instead. */
void	ps1_octal(t_string *out, const char *f, int *i)
{
	int	val;
	int	n;

	val = 0;
	n = 0;
	*i += 1;
	while (n < 3 && f[*i] >= '0' && f[*i] <= '7')
	{
		val = val * 8 + (f[*i] - '0');
		*i += 1;
		n++;
	}
	if (val != 0)
		vec_push_char(out, (char)val);
}

/* ${...}: hand the body to the shell's own parameter expander, so every
   form the word expander already knows -- ${v:-d}, ${v:+w}, ${#v}, ${v#p}
   -- means in a prompt what it means in a command. The reader this replaces
   understood a bare name only: on ${debian_chroot:+(...)} it stopped at the
   ':' and let the rest of the expression reach the screen as text. A NULL
   from the format expander means "no operator here", so a plain ${NAME}
   falls back to a straight lookup. An unterminated ${ keeps the '$'
   literal, matching bash. */
void	ps1_brace(t_shell *state, t_string *out, const char *f, int *i)
{
	int		close;
	char	*fmt;
	char	*env;

	close = ps1_brace_span(f + *i + 1);
	if (close == 0)
		return ((void)vec_push_char(out, '$'), (void)(*i += 1));
	fmt = expand_param_format(state, f + *i + 2, close - 1, true);
	env = NULL;
	if (!fmt)
		env = env_expand_n(state, (char *)f + *i + 2, close - 1);
	*i += close + 2;
	if (fmt)
		return ((void)vec_push_str(out, fmt), xfree(fmt));
	if (env)
		vec_push_str(out, env);
}

/* Is `c` a single-character special parameter -- $?, $$, $!, $#, $1 ...?
**
** This lives here, and BOTH the loop below and ps1_dollar() ask it, because
** they used to disagree. ps1_dollar was taught to read specials while this
** loop still tested only is_var_name_p1(), so `$?` never reached the reader
** at all and rendered as literal text anyway. Two guards for one question is
** how that happens; one predicate is why it cannot happen again. */
bool	ps1_is_special(char c)
{
	return (c && ft_strchr("?$!#-*@0123456789", c) != NULL);
}

/* PROMPT (zsh syntax) -> the backslash language -> the one renderer.
   Kept here rather than in prompt_zsh.c so that file stays a pure
   translator with no dependency on the animation layer. strict selects
   exact-zsh semantics (PROMPT, print -P, a zsh-armed PS1) over the
   bilingual PS1 rules, where an unknown `%` stays literal so a legacy
   percent survives. */
t_string	zsh_prompt(t_shell *state, char *fmt, bool strict)
{
	t_string	conv;
	t_string	out;

	conv = zsh_to_ps1(state, fmt, strict);
	out = ps1_animated(state, (char *)conv.ctx);
	xfree(conv.ctx);
	return (out);
}
