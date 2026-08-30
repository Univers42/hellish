/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_xform2.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:25:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:25:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

t_string	zsh_prompt(t_shell *state, char *fmt);

/* (%): render prompt escapes.  Routed through the same zsh_prompt() the
   PROMPT variable uses, so `${(%)...}` and a prompt string containing the
   same text can never disagree -- which they would the moment there were
   two renderers. */
static char	*zx_prompt(t_shell *state, char *v)
{
	t_string	r;

	r = zsh_prompt(state, v);
	xfree(v);
	if (!r.ctx)
		return (ft_strdup(""));
	return ((char *)r.ctx);
}

/* (P): the value is the NAME of a parameter, and the answer is that
   parameter's value.  zsh's spelling of the indirection bash writes
   ${!name}, and the reason `${(P)var}` shows up in theme code that stores a
   colour in a variable named by another variable. */
static char	*zx_indirect(t_shell *state, char *v)
{
	char	*target;
	char	*out;

	target = env_expand(state, v);
	out = ft_strdup("");
	if (target)
		out = (xfree(out), ft_strdup(target));
	xfree(v);
	return (out);
}

/* One element through the per-element flags.  Owns `v`, returns a fresh
   string; each step frees what the previous one produced, so a chain like
   (PU%q) never leaks an intermediate. */
char	*zx_one(t_shell *state, t_zflags *f, char *v)
{
	char	*next;

	if (zf_has(f, 'P'))
		v = zx_indirect(state, v);
	next = NULL;
	if (zf_has(f, 'U'))
		next = zf_case(v, 'U');
	else if (zf_has(f, 'L'))
		next = zf_case(v, 'L');
	else if (zf_has(f, 'C'))
		next = zf_case(v, 'C');
	if (next)
		v = (xfree(v), next);
	if (zf_has(f, '%'))
		v = zx_prompt(state, v);
	if (zf_has(f, 'q'))
	{
		next = zf_quote(v, zq_style(f));
		v = (xfree(v), next);
	}
	return (v);
}
