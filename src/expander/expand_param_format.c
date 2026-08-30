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
#include "parena.h"
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
	if (wlen <= 0)
		return (ft_strdup(""));
	if (dq)
		return (expand_param_word_dq(state, word, wlen));
	return (pf_word_pipeline(state, word, wlen, false));
}

static bool	is_unset_or_null(const char *val)
{
	return (val == NULL || *val == '\0');
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
