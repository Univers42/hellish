/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_zsh6.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 04:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 04:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Longest truncation replacement string kept (zsh imposes none; themes
   use two or three dots). Named because norminette rejects the
   (int)sizeof spelling. */
#define ZREP_CAP 63

/* zsh prompt truncation:
**
**     %10<...<%~ %<<        the path, at most 10 columns, cut on the LEFT
**     %10>...>text%>>       the same, cut on the right
**
** Measured on the oracle: %10<...<abcdefghijklmnop%<< is "...jklmnop" --
** the replacement string counts toward the budget, and the segment runs
** to the empty %<< (or %>>) or the end of the prompt.
**
** Implemented by rendering the segment through the ordinary pipeline,
** then cutting the RESULT: truncation is about on-screen columns, which
** only exist after expansion. One documented approximation: when a cut
** actually happens, zero-width \001..\002 regions inside the segment
** (colour changes, mostly) are dropped rather than repositioned -- themes
** colour around their truncated path, not inside it. A segment that fits
** is passed through untouched, colours and all. The old %[x...] spelling
** is parsed and ignored.
*/

/* Where the segment ends: the next empty %<< / %>>, or end of string.
   *skip is how many bytes that terminator itself occupies. */
static int	trunc_end(const char *f, int j, int *skip)
{
	while (f[j])
	{
		if (f[j] == '%' && (f[j + 1] == '<' || f[j + 1] == '>')
			&& f[j + 2] == f[j + 1])
			return (*skip = 3, j);
		if (f[j] == '%' && f[j + 1])
			j += 2;
		else
			j++;
	}
	return (*skip = 0, j);
}

/* Copy `s` skipping \001..\002 regions and ESC[...x sequences; returns
   the visible length. `buf` must be as large as `s`. */
static int	trunc_plain(const char *s, char *buf)
{
	int	i;
	int	k;

	i = 0;
	k = 0;
	while (s[i])
	{
		if (s[i] == '\001')
		{
			while (s[i] && s[i] != '\002')
				i++;
			i += (s[i] == '\002');
		}
		else if (s[i] == '\033')
		{
			i++;
			while (s[i] && !ft_isalpha(s[i]))
				i++;
			i += (s[i] != '\0');
		}
		else
			buf[k++] = s[i++];
	}
	return (buf[k] = '\0', k);
}

/* The cut itself: keep `keep` visible characters from the left or the
   right of `plain`, with `rep` standing in for what fell off. */
static void	trunc_emit(t_string *out, char *plain, char *rep, t_zesc *z)
{
	int	vis;
	int	keep;

	vis = ft_strlen(plain);
	keep = z->n - ft_strlen(rep);
	if (keep < 0)
		keep = 0;
	if (z->dir == '<')
	{
		zsh_inject(out, rep);
		zsh_inject(out, plain + vis - keep);
		return ;
	}
	plain[keep] = '\0';
	zsh_inject(out, plain);
	zsh_inject(out, rep);
}

/* Render the segment, then decide whether anything must be cut at all. */
static void	trunc_apply(t_shell *state, t_string *out, char *rep, t_zesc *z)
{
	char		*sub;
	char		*plain;
	t_string	conv;
	t_string	txt;
	int			end[2];

	end[0] = trunc_end(z->f, z->j, &end[1]);
	sub = ft_substr(z->f, z->j, end[0] - z->j);
	z->j = end[0] + end[1];
	if (!sub)
		return ;
	conv = zsh_to_ps1(state, sub);
	txt = ps1_render(state, (char *)conv.ctx);
	plain = xmalloc(txt.len + 1);
	if (z->n > 0 && trunc_plain((char *)txt.ctx, plain) > z->n)
		trunc_emit(out, plain, rep, z);
	else if (txt.ctx)
		zsh_inject(out, (char *)txt.ctx);
	xfree(plain);
	xfree(txt.ctx);
	xfree(conv.ctx);
	xfree(sub);
}

bool	zsh_trunc(t_shell *state, t_string *out, t_zesc *z, char c)
{
	char	rep[ZREP_CAP + 1];
	int		k;

	if (c != '<' && c != '>' && c != '[')
		return (false);
	if (c == '[')
	{
		while (z->f[z->j] && z->f[z->j] != ']')
			z->j++;
		return (z->j += (z->f[z->j] == ']'), true);
	}
	k = 0;
	while (z->f[z->j] && z->f[z->j] != c)
	{
		if (k < ZREP_CAP)
			rep[k++] = z->f[z->j];
		z->j++;
	}
	rep[k] = '\0';
	z->j += (z->f[z->j] == c);
	z->dir = c;
	trunc_apply(state, out, rep, z);
	return (true);
}
