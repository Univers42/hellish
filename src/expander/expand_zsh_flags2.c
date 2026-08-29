/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_flags2.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:05:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:05:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "parena.h"

bool	zf_has(const t_zflags *f, char c)
{
	int	i;

	i = 0;
	while (i < f->n)
	{
		if (f->set[i] == c)
			return (true);
		i++;
	}
	return (false);
}

void	zf_free(t_zflags *f)
{
	xfree(f->sep);
	xfree(f->join);
	f->sep = NULL;
	f->join = NULL;
}

/* The operand after the flags.  zsh reads a bare word there as a PARAMETER
   NAME -- ${(U)path} is the value of $path uppercased -- but anything
   starting with $, a quote or a backtick is an expansion in its own right,
   which is what makes ${(f)$(cmd)} the idiom it is.  The second case runs
   the ordinary word pipeline (tilde, command substitution, parameters; no
   split, no glob); the first re-enters expand_param_format so ${(U)x:-y}
   keeps working, and falls back to a plain lookup.
     env_expand_n's result is BORROWED from the environment, so it is copied:
   everything downstream of here owns what it holds. */
char	*zf_inner(t_shell *state, t_token *tt, const char *s, int slen)
{
	char	*v;

	if (slen <= 0)
		return (ft_strdup(""));
	if (zf_is_nested(s, slen))
	{
		v = zf_nested(state, tt, s, slen);
		if (v)
			return (zn_subscript(state, v, s, slen));
	}
	if (s[0] == '$' || s[0] == '"' || s[0] == '\'' || s[0] == '`')
		return (pf_word_pipeline(state, s, slen, false));
	v = expand_param_format(state, s, slen, false);
	if (v)
		return (v);
	v = env_expand_n(state, (char *)s, slen);
	if (!v)
		return (ft_strdup(""));
	return (ft_strdup(v));
}

/* Loud failure.  An unimplemented flag names itself before the shell reports
   the substitution as bad, because "bad substitution" alone sends the reader
   hunting for a typo in a line that has none.  pf_bad_subst then does what
   bash does with any malformed ${...}: status 127, and exit for a
   non-interactive shell -- which is the point.  A plugin that quietly got
   the unflagged value back would produce a wrong answer nobody could trace,
   and would only be found by whatever it went on to break. */
void	zf_bad(t_shell *state, t_token *tt, char flag)
{
	char	*v;

	if (flag)
		ft_eprintf("%s: ${(%c)...}: zsh flag not implemented\n",
			state->ctx, flag);
	v = pf_bad_subst(state, tt->start, tt->len);
	tt->start = v;
	tt->len = (int)ft_strlen(v);
	tt->allocated = true;
	parena_note_attach();
}

/* Split, order, transform, emit -- zsh's own pipeline order, and the reason
   the internal representation is a list even for `${(U)x}`.  Owns `val`. */
void	zf_finish(t_shell *state, t_zflags *f, t_token *tt, char *val)
{
	t_vec	list;

	if (!zf_check(state, f, tt))
		return ((void)xfree(val));
	zf_unesc(f);
	list = zl_from(state, f, val);
	xfree(val);
	zl_order(f, &list);
	zl_map(state, f, &list);
	zf_emit(state, f, tt, &list);
}
