/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_zsh7.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 06:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 06:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Longest raw escape spelling re-emitted literally ('%', a numeric
   argument, one character); named because norminette rejects the
   (int)sizeof spelling. */
#define ZLIT_CAP 15

/* The BILINGUAL half of the reader. PS1 renders both escape languages at
** once -- users who spent twenty years typing PS1 will not discover that
** zsh syntax "belongs" in PROMPT, so PS1 has to just take it. What keeps
** that from eating anyone's legacy percent is a different rule set from
** strict zsh, applied only when strict is off:
**
**   * an UNKNOWN or MALFORMED `%` sequence stays on screen literally
**     (strict zsh consumes it -- measured), so `100% ` and a csh-style
**     `%> ` survive;
**   * $(...) / ${...} / $((...)) / \D{...} spans and every backslash
**     pair are copied through untouched, so the strftime percents in
**     `$(date +%H:%M)` or `\D{%M}` can never be read as %H/%M escapes.
**
** PROMPT and a zsh-armed PS1 keep exact zsh semantics; the parity suite
** pins those against the 5.9 oracle and none of this file applies there.
*/

/* The one-character escapes that map straight onto a backslash spelling.
   %h and %! are the history number; %N/%x answer "what file am I", which
   is how `${(%):-%N}` opens half the plugin corpus. */
const char	*zsh_simple(char c)
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

/* Where a copy-verbatim span starting at i ends, or i when there is none.
   Bracket matching is by count, not kind -- a prompt string is not a
   parser's input, and the sloppiness cannot leak past the span. */
int	zsh_span_dollar(const char *f, int i)
{
	int	depth;
	int	j;

	if (f[i] == '\\' && f[i + 1] == 'D' && f[i + 2] == '{')
	{
		j = i + 3;
		while (f[j] && f[j] != '}')
			j++;
		return (j + (f[j] == '}'));
	}
	if (f[i] == '\\' && f[i + 1])
		return (i + 2);
	if (f[i] != '$' || (f[i + 1] != '(' && f[i + 1] != '{'))
		return (i);
	depth = 0;
	j = i + 1;
	while (f[j])
	{
		if (f[j] == '(' || f[j] == '{')
			depth++;
		if ((f[j] == ')' || f[j] == '}') && --depth == 0)
			return (j + 1);
		j++;
	}
	return (j);
}

/* Copy one such span into the output, cursor included. False = no span. */
bool	zsh_span_copy(t_string *out, const char *f, int *i)
{
	int	j;

	j = zsh_span_dollar(f, *i);
	if (j == *i)
		return (false);
	while (*i < j)
		vec_push_char(out, f[(*i)++]);
	return (true);
}

/* Emit the escape's own spelling -- `%`, the numeric argument if one was
   written, the character -- through the injection escaper, so a `$` or
   `\` in it cannot be re-read by the renderer. Only the mixed reader
   calls this; strict zsh renders an unknown escape as nothing. */
void	zsh_lit(t_string *out, t_zesc *z)
{
	char	buf[ZLIT_CAP + 1];
	int		i;

	vec_push_char(out, '%');
	i = z->start + 1;
	while (i < z->j && i - z->start - 1 < ZLIT_CAP)
	{
		buf[i - z->start - 1] = z->f[i];
		i++;
	}
	buf[i - z->start - 1] = '\0';
	zsh_inject(out, buf);
}
