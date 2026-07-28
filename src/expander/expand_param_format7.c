/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_format7.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* Thin wrapper: look up `name[0..len)` in the environment.  Returns NULL if
   the variable is unset (distinct from set-but-empty, which returns "").
   Centralised here so every expand_param_* helper uses the same lookup. */
char	*pf_get_var_value(t_shell *state, const char *name, int len)
{
	return (env_expand_n(state, (char *)name, len));
}

/* Route a trim/substitution ctx to the right evaluator (split out of
   expand_param_format to keep it inside the norm line budget). */
char	*pf_trim_or_subst(t_shell *state, t_trim_ctx ctx)
{
	if (*ctx.op == '/')
		return (expand_subst(state, ctx));
	return (expand_trim(state, ctx));
}

/* Assemble the trim/subst context handed to the evaluators. */
t_trim_ctx	pf_make_ctx(const char *s, int slen, const char *op, int nlen)
{
	t_trim_ctx	ctx;

	ctx.name = s;
	ctx.name_len = nlen;
	ctx.op = op;
	ctx.slen = slen;
	return (ctx);
}

/* Expand an operator word that sits inside double quotes.  Rather than
   duplicating the reparser's dq escape rules, wrap the raw word in a pair
   of double quotes and run it through the normal unquoted pipeline: the
   reparser then applies exactly the "..." semantics — \$ \" \\ \` stay
   active escapes, any other backslash is kept literally ("${u-\z}" prints
   \z), tilde stays literal, and embedded quotes behave as they do in bash
   ("${u-'a b'}" keeps the single quotes). */
char	*expand_param_word_dq(t_shell *state, const char *word, int wlen)
{
	char	*buf;
	char	*ret;

	buf = xmalloc((size_t)wlen + 3);
	buf[0] = '"';
	ft_memcpy(buf + 1, word, (size_t)wlen);
	buf[wlen + 1] = '"';
	buf[wlen + 2] = '\0';
	ret = expand_param_word(state, buf, wlen + 2, false);
	xfree(buf);
	return (ret);
}
