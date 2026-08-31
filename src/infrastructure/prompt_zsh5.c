/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_zsh5.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 04:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 04:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "env.h"

/* %(x.true-text.false-text): the zsh prompt conditional, evaluated here at
** rewrite time and REPLACED by its chosen branch, which is then fed back
** through the rewriter -- conditionals nest, and the oracle's
** %(?.a%(!.b.c)d.e) -> acd is a test case.
**
** The delimiter is whatever character follows the condition (themes use
** '.', ':' and stranger), and the scan is nesting-aware: a bare ')' at
** depth belongs to an inner conditional, not to us.
**
** Conditions understood: ? (status is n), ! (privileged), # (euid is n),
** j (at least n jobs), L (SHLVL at least n), C (at least n absolute path
** components), c/~/. (the same, ~-relative), e (eval depth at least n).
** The rest of zsh's letters (t/T/d/D/w clock comparisons, S seconds,
** g gid, l line length, v psvar size, _ parser depth) evaluate FALSE and
** are listed here so the gap is a documented decision, not a surprise. */

/* Advance *j to the next delimiter or ')' at nesting depth zero; a `%`
   escapes the character after it, and `%(` opens a nested conditional.
   Returns what stopped the scan, or 0 at end of string. */
static char	cond_next(const char *f, int *j, char delim)
{
	int	depth;

	depth = 0;
	while (f[*j])
	{
		if (f[*j] == '%' && f[*j + 1])
		{
			if (f[*j + 1] == '(')
				depth++;
			*j += 2;
			continue ;
		}
		if (f[*j] == ')' && depth > 0)
			depth--;
		else if (depth == 0 && (f[*j] == delim || f[*j] == ')'))
			return (f[*j]);
		(*j)++;
	}
	return (0);
}

static bool	cond_eval(t_shell *state, int n, char x)
{
	char	*lvl;

	if (x == '?')
		return (state->last_cmd_st_exe.status == n);
	if (x == '!')
		return (geteuid() == 0);
	if (x == '#')
		return ((int)geteuid() == n);
	if (x == 'j')
		return ((int)state->job_table.count >= n);
	if (x == 'e')
		return ((int)state->call_frames.len >= n);
	if (x == 'L')
	{
		lvl = env_expand(state, "SHLVL");
		return (lvl && ft_atoi(lvl) >= n);
	}
	if (x == 'C')
		return (zsh_cond_comps(state, false) >= n);
	if (x == 'c' || x == '~' || x == '.')
		return (zsh_cond_comps(state, true) >= n);
	return (false);
}

/* Rewrite the span sp[0]..sp[1] of the format as its own little prompt
   and append the result -- the recursion that makes nesting work, with
   the caller's strictness carried through. */
static void	cond_branch(t_shell *state, t_string *out, t_zesc *z, int *sp)
{
	char		*sub;
	t_string	conv;

	sub = ft_substr(z->f, sp[0], sp[1] - sp[0]);
	if (!sub)
		return ;
	conv = zsh_to_ps1(state, sub, z->strict);
	if (conv.ctx)
		vec_push_str(out, (char *)conv.ctx);
	xfree(conv.ctx);
	xfree(sub);
}

/* The two-branch tail, once a real delimiter was found: scan the false
   part to the closing ')', evaluate, splice the winner. */
static bool	cond_two(t_shell *state, t_string *out, t_zesc *z, int *sp)
{
	sp[2] = sp[1] + 1;
	sp[3] = sp[2];
	cond_next(z->f, &sp[3], ')');
	if (cond_eval(state, z->n * z->has_n, z->f[z->j]))
		cond_branch(state, out, z, sp);
	else
		cond_branch(state, out, z, sp + 2);
	return (z->j = sp[3] + (z->f[sp[3]] == ')'), true);
}

bool	zsh_cond(t_shell *state, t_string *out, t_zesc *z, char c)
{
	int		sp[4];

	if (c != '(')
		return (false);
	zsh_num(z);
	if (!z->f[z->j] || !z->f[z->j + 1])
	{
		if (!z->strict)
			return (zsh_lit(out, z), true);
		return (z->j += ft_strlen(z->f + z->j), true);
	}
	sp[0] = z->j + 2;
	sp[1] = sp[0];
	if (cond_next(z->f, &sp[1], z->f[z->j + 1]) != z->f[z->j + 1])
	{
		if (cond_eval(state, z->n * z->has_n, z->f[z->j]))
			cond_branch(state, out, z, sp);
		return (z->j = sp[1], true);
	}
	return (cond_two(state, out, z, sp));
}
