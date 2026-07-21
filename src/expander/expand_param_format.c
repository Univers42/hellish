/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_format.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 09:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 09:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "sh_input.h"
#include "decomposer.h"

void	exit_clean(t_shell *state, int code);

/* Expand the word part of a ${name:-word} or similar operator.  The word
   goes through a full tilde/command-subst/parameter pass but NOT field
   splitting or globbing (POSIX says the word is expanded in double-quote
   context).  This is what makes ${VAR:-~/bin} expand the ~ correctly while
   ${VAR:-*.c} does NOT produce a glob list.  `dq` marks a ${...} that sits
   inside double quotes: the word then keeps the dq backslash rules ("\z"
   stays "\z") via the wrap in expand_param_word_dq instead of the unquoted
   reparse below which strips the escape. */
char	*expand_param_word(t_shell *state, const char *word, int wlen, bool dq)
{
	t_ast_node	w;
	t_token		t;
	t_string	s;
	char		*ret;

	if (wlen <= 0)
		return (ft_strdup(""));
	if (dq)
		return (expand_param_word_dq(state, word, wlen));
	t.start = (char *)word;
	t.len = wlen;
	t.tt = TT_WORD;
	w = reparse_word(t);
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

/* Thin wrapper: look up `name[0..len)` in the environment.  Returns NULL if
   the variable is unset (distinct from set-but-empty, which returns "").
   Centralised here so every expand_param_* helper uses the same lookup. */
char	*pf_get_var_value(t_shell *state, const char *name, int len)
{
	return (env_expand_n(state, (char *)name, len));
}

static bool	is_unset_or_null(const char *val)
{
	return (val == NULL || *val == '\0');
}

/* ${#arr[@]} is the element count, ${#arr[i]} an element's length; the
   subscript token is expanded by expand_array_token first, so here we
   only need the name[body] shapes on the raw text. */
static char	*expand_strlen_arr(t_shell *state, const char *s, int slen)
{
	t_token	tok;

	tok = (t_token){.tt = TT_ENVVAR, .start = (char *)s, .len = slen};
	if (slen > 3 && s[slen - 1] == ']' && (s[slen - 2] == '@'
			|| s[slen - 2] == '*') && s[slen - 3] == '[')
	{
		tok.start = env_expand_n(state, (char *)s, slen - 3);
		if (tok.start && !arr_is(tok.start))
			return (ft_itoa(1));
		return (ft_itoa(arr_count(tok.start)));
	}
	if (!expand_array_token(state, &tok, false))
		return (NULL);
	return (ft_itoa(tok.len));
}

/* ${#param} — return the length of the variable's value as a decimal string.
   An unset variable counts as length 0 rather than an error (unless set -u).
   The result is always a fresh malloc'd string (ft_itoa allocates). */
char	*expand_strlen(t_shell *state, const char *s, int slen)
{
	char	*val;
	char	*result;

	if (ft_strnchr((char *)s, '[', slen))
	{
		result = expand_strlen_arr(state, s, slen);
		if (result)
			return (result);
	}
	val = pf_get_var_value(state, s, slen);
	if (!val)
		return (ft_strdup("0"));
	if (arr_is(val))
		val = arr_get_idx(val, 0);
	if (!val)
		return (ft_strdup("0"));
	result = ft_itoa(ft_strlen(val));
	return (result);
}

/* Handle the default/alternate family of parameter operators:
     ${p-w}  → w if p is UNSET,          else p
     ${p:-w} → w if p is UNSET OR EMPTY, else p
     ${p+w}  → "" if p is UNSET,         else w
     ${p:+w} → "" if p is UNSET OR EMPTY, else w
   The colon variant (o.colon) treats an empty string the same as unset.
   `val` is NULL for unset, "" for set-but-empty (pf_get_var_value contract). */
char	*default_or_alt(t_shell *state, char *val, t_pe_op o)
{
	bool	act;

	if (o.colon)
		act = is_unset_or_null(val);
	else
		act = (val == NULL);
	if (o.opc == '-')
	{
		if (act)
			return (expand_param_word(state, o.word, o.wlen, o.dq));
		return (ft_strdup(val));
	}
	if (act)
		return (ft_strdup(""));
	return (expand_param_word(state, o.word, o.wlen, o.dq));
}
