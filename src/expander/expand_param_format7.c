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
#include "decomposer.h"

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
   ("${u-'a b'}" keeps the single quotes).

   One escape has to be handled here rather than by the wrap: \} .  Inside
   ${...} a backslash before the closing brace is ACTIVE -- it is the only
   way to put a literal } in an operator word -- so bash prints } for
   "${u-\}}" while the plain dq rule would keep the slash.  \{ is not
   special and stays, and \\ is left alone so "${u-\\}" still prints one
   backslash. */
char	*expand_param_word_dq(t_shell *state, const char *word, int wlen)
{
	char	*buf;
	char	*ret;
	int		i;
	int		n;

	buf = xmalloc((size_t)wlen + 3);
	buf[0] = '"';
	i = 0;
	n = 1;
	while (i < wlen)
	{
		if (word[i] == '\\' && i + 1 < wlen && word[i + 1] == '}')
			i++;
		buf[n++] = word[i++];
	}
	buf[n++] = '"';
	buf[n] = '\0';
	ret = pf_word_pipeline(state, buf, n, true);
	xfree(buf);
	return (ret);
}

/* Reparse one operator word and run the expansions it may contain, then
   flatten it back to a string.  no_sq asks the reparser to leave single
   quotes alone; the dq wrapper above needs that because a ' inside
   "${...}" is an ordinary character even where the word's own quotes have
   toggled the quoting back off, and a lone one used to run the scan off
   the end of the word and trip the reparser's closing-quote assertion. */
char	*pf_word_pipeline(t_shell *state, const char *word, int wlen,
			bool no_sq)
{
	t_ast_node	w;
	t_token		t;
	t_string	s;
	char		*ret;

	t.start = (char *)word;
	t.len = wlen;
	t.tt = TT_WORD;
	w = reparse_word(t, no_sq);
	expand_tilde_word(state, &w);
	expand_cmd_substitutions(state, &w);
	expand_env_vars(state, &w, false);
	s = word_to_string(w);
	if (!s.ctx)
		ret = ft_strdup("");
	else
		ret = ft_strndup((char *)s.ctx, s.len);
	xfree(s.ctx);
	free_ast(&w);
	return (ret);
}
