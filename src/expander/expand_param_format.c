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
#include "sh_input.h"
#include "decomposer.h"

void	exit_clean(t_shell *state, int code);

/* Expand the word part of a ${name-word} form: tilde, command/arith and
   parameter (incl. nested ${...}) expansion, but no splitting/globbing. */
char	*expand_param_word(t_shell *state, const char *word, int wlen)
{
	t_ast_node	w;
	t_token		t;
	t_string	s;
	char		*ret;

	if (wlen <= 0)
		return (ft_strdup(""));
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
	free(s.ctx);
	free_ast(&w);
	return (ret);
}

char	*pf_get_var_value(t_shell *state, const char *name, int len)
{
	return (env_expand_n(state, (char *)name, len));
}

static bool	is_unset_or_null(const char *val)
{
	return (val == NULL || *val == '\0');
}

/*
** Handle ${#parameter} - string length
** Returns the length of the value as a string, or "0" if unset.
*/
char	*expand_strlen(t_shell *state, const char *s, int slen)
{
	char	*val;
	char	*result;

	val = pf_get_var_value(state, s, slen);
	if (!val)
		return (ft_strdup("0"));
	result = ft_itoa(ft_strlen(val));
	return (result);
}

/*
** ${param-word} ${param:-word} ${param+word} ${param:+word}
** With a colon, "unset OR null" triggers; without, only "unset" triggers.
*/
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
			return (expand_param_word(state, o.word, o.wlen));
		return (ft_strdup(val));
	}
	if (act)
		return (ft_strdup(""));
	return (expand_param_word(state, o.word, o.wlen));
}
