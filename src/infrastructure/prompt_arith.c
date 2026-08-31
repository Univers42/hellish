/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_arith.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 23:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 23:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "arith.h"

/* $((expr)) inside a prompt.
**
** bash evaluates it; hellish rendered `$((2*21))` as those nine characters
** on screen, because ps1_dollar only ever recognised a NAME after the
** dollar and `(` is not one.  Found by previewing a theme that used it --
** the prompt looked like a shell that had forgotten how to do arithmetic.
**
** Not the same call as command substitution, which the prompt renderer
** deliberately does NOT run: `$(cmd)` forks, touches the filesystem, and
** costs a process on every keystroke that redraws.  `$((expr))` does none
** of that -- it reads variables and returns a number -- so the reason to
** exclude one is not a reason to exclude the other.
**
** Malformed input keeps the dollar literal and lets the rest of the prompt
** through, the same fallback every other branch of ps1_dollar takes: a
** prompt that vanishes because of a typo in it is unusable, and a prompt
** that shows the typo can be fixed.
**
** prompt_depth is why `PS1='$((1/0))'` reports and carries on instead of
** ending the session.  arith_fail exits a non-interactive shell, which is
** right for a script and catastrophic here -- the prompt is what draws the
** session you would fix the typo in.  bash reports and leaves the text
** literal; so does this.
*/

/* Index just past the closing `))`, or -1 when it is missing.  Counts nested
   parens so `$(( (1+2) * 3 ))` finds its own end rather than the first one
   it meets. */
static int	arith_end(const char *f, int start)
{
	int	depth;
	int	i;

	depth = 0;
	i = start;
	while (f[i])
	{
		if (f[i] == '(')
			depth++;
		else if (f[i] == ')')
		{
			depth--;
			if (depth == 0)
				return (i + 1);
		}
		i++;
	}
	return (-1);
}

/* True when `f + *i` opens an arithmetic expansion; renders it and advances
   *i past it.  False leaves both untouched for the caller's other cases. */
bool	ps1_arith(t_shell *state, t_string *out, const char *f, int *i)
{
	int		end;
	char	*val;

	if (f[*i + 1] != '(' || f[*i + 2] != '(')
		return (false);
	end = arith_end(f, *i + 1);
	if (end < 0 || f[end - 2] != ')')
		return (false);
	state->prompt_depth++;
	val = arith_expand(state, f + *i + 3, end - *i - 5);
	state->prompt_depth--;
	if (!val)
		return (false);
	vec_push_str(out, val);
	xfree(val);
	*i = end;
	return (true);
}
