/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_op_at.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/19 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Scan a parameter name at the start of a ${...} body: the aggregates @ and
   *, a digit-run positional, or an identifier.  Returns the name length, or
   0 when the body does not start with a parameter name.  Shared by
   find_param_op; the trim/subst scanners keep their own identifier-only
   scan so ${@%x} still routes to bad substitution unchanged. */
int	pf_scan_name(const char *s, int slen)
{
	int	i;

	i = 0;
	if (i < slen && (s[i] == '@' || s[i] == '*'))
		return (1);
	if (i < slen && ft_isdigit((unsigned char)s[i]))
	{
		while (i < slen && ft_isdigit((unsigned char)s[i]))
			i++;
		return (i);
	}
	if (i < slen && (s[i] == '_' || ft_isalpha((unsigned char)s[i])))
	{
		i++;
		while (i < slen && (s[i] == '_' || ft_isalnum((unsigned char)s[i])))
			i++;
	}
	return (i);
}

/* The unset/null test for ${@-w} family operators, bash semantics.  Without
   a colon only "no positional parameters at all" triggers.  With a colon the
   positionals count as null when the field list they would produce is empty
   or a single empty field — except quoted "$*", which joins into ONE field
   first, so with IFS= two empty positionals join to "" and DO count as null
   (spec: "$*" ("" "") and - and + (IFS=)) while unquoted $* does not. */
static bool	at_cond(t_shell *state, t_pe_op o)
{
	char	*j;
	bool	r;

	if (!o.colon)
		return (state->pos.count == 0);
	if (o.dq && o.name[0] == '*')
	{
		j = join_positionals(state);
		r = (j == NULL || j[0] == '\0');
		xfree(j);
		return (r);
	}
	if (state->pos.count == 0)
		return (true);
	return (state->pos.count == 1 && state->pos.args[0][0] == '\0');
}

/* Install an allocated string as the token's expansion result. */
static void	at_set_word(t_token *tt, char *s)
{
	tt->start = s;
	tt->len = (int)ft_strlen(s);
	tt->allocated = true;
}

/* The operator does not substitute: the token must behave exactly like a
   plain $@ / $* would.  Mirror of expand_positional in expand_token.c:
   deferred tokens point at the pos_mark() sentinel (POINTER identity is
   what split_one_child recognises) so it re-emits one field per
   positional later — "$@" verbatim, unquoted $@/$* each IFS-split;
   everything else joins with the first IFS character right here.  This
   is what makes "${@-x}" with two positionals still produce two fields. */
static void	at_pass(t_shell *state, t_token *tt, char name, bool split_ctx)
{
	char	*tmp;

	tt->allocated = false;
	tt->len = 1;
	tt->start = (char *)pos_mark(name);
	if (split_ctx && tt->tt == TT_DQENVVAR && name == '@')
		return ;
	if (split_ctx && tt->tt == TT_ENVVAR)
		return ;
	tmp = join_positionals(state);
	tt->start = tmp;
	tt->len = (int)ft_strlen(tmp);
	tt->allocated = true;
}

/* ${@-w} ${@+w} ${@?w} ${@=w} and the * twins, token context.  When the
   operator substitutes, the word is a single field (expanded with the
   enclosing quote context); when it does not, at_pass re-creates plain
   $@/$* behaviour.  A '+' that does NOT take its word also passes through:
   the parameter is null there, so expanding it yields exactly bash's field
   output (zero fields for quoted "$@" with $#=0, one empty field for "$*").
   '=' cannot assign to the aggregates — bash errors out, status 1. */
void	expand_positional_op(t_shell *state, t_token *tt, t_pe_op o,
			bool split_ctx)
{
	bool	cond;

	cond = at_cond(state, o);
	if (o.opc == '?' && cond)
		return (at_set_word(tt, pf_err_word(state, NULL, o)));
	if (o.opc == '=' && cond)
		return (at_set_word(tt, pf_assign_err(state, o)));
	if (o.opc == '-' && cond)
		return (at_set_word(tt,
				expand_param_word(state, o.word, o.wlen, o.dq)));
	if (o.opc == '+' && !cond)
		return (at_set_word(tt,
				expand_param_word(state, o.word, o.wlen, o.dq)));
	at_pass(state, tt, o.name[0], split_ctx);
}
